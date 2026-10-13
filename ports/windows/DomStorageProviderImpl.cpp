// Windows storage implementation for LiteView's in-process Mojo bridge.
// Own the data: Blink's StorageAreaMap can disappear on navigation.
#include "content/browser/DomStorageProviderImpl.h"
#include "content/browser/MbWebview.h"
#include "content/common/CreateAndBindTempl.h"
#include "content/renderer/WebViewClientImpl.h"
#include "content/renderer/RenderThreadImpl.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/public/mojom/dom_storage/dom_storage.mojom-blink.h"
#include "third_party/blink/public/mojom/dom_storage/storage_area.mojom-blink.h"
#include "third_party/blink/public/mojom/dom_storage/session_storage_namespace.mojom-blink.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "base/files/file_util.h"
#include "base/hash/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/pickle.h"
#include "content/QtBridgeHooks.h"
#include <map>
#include <memory>
#include <tuple>

namespace content {
namespace {
namespace mojom = ::blink::mojom::blink;
using Bytes = WTF::Vector<uint8_t>;
using Key = std::tuple<std::string, std::string, std::string>;

std::string bytes(const Bytes& value) {
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}
Bytes vector(const std::string& value) {
    Bytes result;
    result.Append(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    return result;
}
base::FilePath directory(const blink::LocalFrameToken& token) {
    auto frame = blink::LocalFrame::FromFrameToken(token);
    if (!frame || !frame->View()) return {};
    auto chrome = frame->View()->GetChromeClient();
    if (!chrome || !chrome->GetWebView()) return {};
    auto client = static_cast<WebViewClientImpl*>(chrome->GetWebView()->Client());
    return client && client->getMbwebview() ? client->getMbwebview()->getLocalStorageDir() : base::FilePath();
}
struct Area {
    std::map<std::string, std::string> data;
    mojo::RemoteSet<mojom::StorageAreaObserver> observers;
    base::FilePath file;

    void load() {
        if (file.empty()) return;
        std::string content;
        if (!base::ReadFileToStringWithMaxSize(file, &content, 16*1024*1024)) return;
        base::Pickle pickle(content.data(), static_cast<int>(content.size()));
        base::PickleIterator it(pickle);
        uint32_t count;
        if (!it.ReadUInt32(&count) || count > 1000000) return;
        std::map<std::string, std::string> loaded;
        for (uint32_t i = 0; i < count; ++i) {
            std::string key, value;
            if (!it.ReadString(&key) || !it.ReadString(&value)) return;
            loaded.emplace(std::move(key), std::move(value));
        }
        data.swap(loaded);
    }
    void save() {
        if (file.empty()) return;
        base::Pickle pickle;
        pickle.WriteUInt32(static_cast<uint32_t>(data.size()));
        for (const auto& entry : data) {
            pickle.WriteString(entry.first);
            pickle.WriteString(entry.second);
        }
        if (!base::CreateDirectory(file.DirName())) return;
        auto temporary = file.AddExtension(FILE_PATH_LITERAL("tmp"));
        int size = static_cast<int>(pickle.size());
        if (base::WriteFile(temporary, static_cast<const char*>(pickle.data()), size) == size)
            base::ReplaceFile(temporary, file, nullptr);
    }
};
std::map<Key, std::shared_ptr<Area>>& areas() {
    static auto* instance = new std::map<Key, std::shared_ptr<Area>>;
    return *instance;
}
std::shared_ptr<Area> getArea(const blink::BlinkStorageKey& storageKey,
    const blink::LocalFrameToken& token, const WTF::String& nameSpace) {
    auto path = directory(token);
    std::string profile = path.AsUTF8Unsafe();
    std::string origin = storageKey.ToDebugString().Utf8();
    std::string space = nameSpace.Utf8();
    auto& area = areas()[Key(profile, space, origin)];
    if (!area) {
        area = std::make_shared<Area>();
        if (space.empty() && !path.empty() && profile.rfind("qliteview-memory:", 0) != 0) {
            auto digest = base::SHA1HashString(origin);
            area->file = path.AppendASCII(base::HexEncode(digest.data(), digest.size()) + ".qlstorage");
            area->load();
        }
    }
    return area;
}
class StorageArea final : public mojom::StorageArea {
public:
    StorageArea(const blink::BlinkStorageKey& key, const blink::LocalFrameToken& token, const WTF::String& space)
        : area(getArea(key, token, space)), session(!space.empty()) {}

    void AddObserver(mojo::PendingRemote<mojom::StorageAreaObserver> observer) override {
        if (observer.is_valid()) area->observers.Add(std::move(observer));
    }
    void SetStorageAreaMap(void*) override {}
    void Put(const Bytes& key, const Bytes& value, const absl::optional<Bytes>&,
        const WTF::String& source, PutCallback callback) override {
        auto rawKey = bytes(key);
        auto existing = area->data.find(rawKey);
        absl::optional<Bytes> old;
        if (existing != area->data.end()) old = vector(existing->second);
        size_t usage = 0;
        for (const auto& entry : area->data) usage += entry.first.size() + entry.second.size();
        if (existing != area->data.end()) usage -= existing->first.size() + existing->second.size();
        if (usage + key.size() + value.size() > kPerStorageAreaQuota) {
            std::move(callback).Run(false);
            return;
        }
        area->data[rawKey] = bytes(value);
        area->save();
        notify(0, key, value, old, source, true, std::move(callback));
    }
    void Delete(const Bytes& key, const absl::optional<Bytes>&, const WTF::String& source, DeleteCallback callback) override {
        auto found = area->data.find(bytes(key));
        if (found != area->data.end()) {
            auto old = vector(found->second);
            area->data.erase(found);
            area->save();
            notify(1, key, {}, old, source, true, std::move(callback));
            return;
        }
        std::move(callback).Run(true);
    }
    void DeleteAll(const WTF::String& source, mojo::PendingRemote<mojom::StorageAreaObserver> observer, DeleteAllCallback callback) override {
        AddObserver(std::move(observer));
        bool nonempty = !area->data.empty();
        area->data.clear();
        area->save();
        notify(2, {}, {}, absl::nullopt, source, nonempty, std::move(callback));
    }
    void Get(const Bytes& key, GetCallback callback) override {
        auto found = area->data.find(bytes(key));
        std::move(callback).Run(found != area->data.end(), found != area->data.end() ? vector(found->second) : Bytes());
    }
    bool GetAll(mojo::PendingRemote<mojom::StorageAreaObserver> observer, WTF::Vector<mojom::KeyValuePtr>* output) override {
        AddObserver(std::move(observer));
        output->clear();
        for (const auto& entry : area->data)
            output->push_back(mojom::KeyValue::New(vector(entry.first), vector(entry.second)));
        return true;
    }
    void GetAll(mojo::PendingRemote<mojom::StorageAreaObserver> observer, GetAllCallback callback) override {
        WTF::Vector<mojom::KeyValuePtr> output;
        GetAll(std::move(observer), &output);
        std::move(callback).Run(std::move(output));
    }
private:
    void notify(int kind, Bytes key, Bytes value, absl::optional<Bytes> old,
        WTF::String source, bool nonempty, base::OnceCallback<void(bool)> callback) {
        if (session) {
            std::move(callback).Run(true);
            return;
        }
        // Blink enqueues its pending mutation after Put returns. Deliver the
        // observer notification on the next task, retaining data but no view.
        RenderThreadImpl::get()->getTaskRunner()->PostTask(FROM_HERE,
            base::BindOnce([](std::shared_ptr<Area> area, int kind, Bytes key, Bytes value,
                absl::optional<Bytes> old, WTF::String source, bool nonempty,
                base::OnceCallback<void(bool)> callback) {
                for (const auto& observer : area->observers) {
                    if (!observer.TryGet()) continue;
                    if (kind == 0) observer->KeyChanged(key, value, old, source);
                    else if (kind == 1) observer->KeyDeleted(key, old, source);
                    else observer->AllDeleted(nonempty, source);
                }
                std::move(callback).Run(true);
            }, area, kind, std::move(key), std::move(value), std::move(old),
                std::move(source), nonempty, std::move(callback)));
    }
    std::shared_ptr<Area> area;
    bool session;
};
class SessionNamespace final : public mojom::SessionStorageNamespace {
public:
    explicit SessionNamespace(const WTF::String& space) : space(space.Utf8()) {}
    void Clone(const WTF::String& destination) override {
        std::vector<std::pair<Key, std::shared_ptr<Area>>> copies;
        for (const auto& entry : areas()) {
            if (std::get<1>(entry.first) != space) continue;
            auto copy = std::make_shared<Area>();
            copy->data = entry.second->data;
            copies.emplace_back(Key(std::get<0>(entry.first), destination.Utf8(), std::get<2>(entry.first)), copy);
        }
        for (auto& copy : copies) areas()[copy.first] = std::move(copy.second);
    }
private:
    std::string space;
};
class DomStorage final : public mojom::DomStorage {
public:
    void OpenLocalStorage(const blink::BlinkStorageKey& key, const blink::LocalFrameToken& token,
        mojo::PendingReceiver<mojom::StorageArea> receiver) override {
        createAndBindBrokerProxy<mojom::StorageArea, StorageArea>(receiver.PassPipe(), key, token, WTF::String());
    }
    void BindSessionStorageNamespace(const WTF::String& space, mojo::PendingReceiver<mojom::SessionStorageNamespace> receiver) override {
        createAndBindBrokerProxy<mojom::SessionStorageNamespace, SessionNamespace>(receiver.PassPipe(), space);
    }
    void BindSessionStorageArea(const blink::BlinkStorageKey& key, const blink::LocalFrameToken& token,
        const WTF::String& space, mojo::PendingReceiver<mojom::StorageArea> receiver) override {
        createAndBindBrokerProxy<mojom::StorageArea, StorageArea>(receiver.PassPipe(), key, token, space);
    }
};
}
DomStorageProviderImpl::DomStorageProviderImpl() = default;
void DomStorageProviderImpl::BindDomStorage(mojo::PendingReceiver<blink::mojom::blink::DomStorage> receiver,
    mojo::PendingRemote<blink::mojom::blink::DomStorageClient>) {
    createAndBindBrokerProxy<mojom::DomStorage, DomStorage>(receiver.PassPipe());
}
}

std::string qliteview::storageProfile(blink::LocalFrame* frame) {
    return frame ? content::directory(frame->GetLocalFrameToken()).AsUTF8Unsafe() : std::string();
}
