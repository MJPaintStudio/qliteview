#include <windows.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include "v8.h"
#include "libplatform/libplatform.h"
#include "unicode/udata.h"

extern unsigned char SnapshotBlobBinX64[471944];
extern unsigned char icudtlData[];
extern "C" {
extern const uint8_t v8_Default_embedded_blob_data_stub[];
extern const uint8_t v8_Default_embedded_blob_code_stub[];
extern uint32_t v8_Default_embedded_blob_data_size_;
extern uint32_t v8_Default_embedded_blob_code_size_;
const uint8_t* v8_Default_embedded_blob_data_ = nullptr;
const uint8_t* v8_Default_embedded_blob_code_ = nullptr;
}

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    UErrorCode error = U_ZERO_ERROR;
    udata_setCommonData(icudtlData, &error);
    if (U_FAILURE(error)) { std::fprintf(stderr, "ICU: %d\n", error); return 1; }
    auto* code = static_cast<uint8_t*>(VirtualAlloc(nullptr,
        v8_Default_embedded_blob_code_size_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!code) return 2;
    std::memcpy(code, v8_Default_embedded_blob_code_stub, v8_Default_embedded_blob_code_size_);
    DWORD oldProtection;
    if (!VirtualProtect(code, v8_Default_embedded_blob_code_size_, PAGE_EXECUTE_READ, &oldProtection)) return 3;
    FlushInstructionCache(GetCurrentProcess(), code, v8_Default_embedded_blob_code_size_);
    v8_Default_embedded_blob_code_ = code;
    v8_Default_embedded_blob_data_ = v8_Default_embedded_blob_data_stub;
    v8::StartupData snapshot{reinterpret_cast<const char*>(SnapshotBlobBinX64), sizeof(SnapshotBlobBinX64)};
    v8::V8::SetSnapshotDataBlob(&snapshot);
    auto platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    if (!v8::V8::Initialize()) return 4;
    std::unique_ptr<v8::ArrayBuffer::Allocator> allocator(v8::ArrayBuffer::Allocator::NewDefaultAllocator());
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = allocator.get();
    auto* isolate = v8::Isolate::New(params);
    bool passed = false;
    {
        v8::Isolate::Scope isolateScope(isolate);
        v8::HandleScope handleScope(isolate);
        auto context = v8::Context::New(isolate);
        v8::Context::Scope contextScope(context);
        auto source = v8::String::NewFromUtf8Literal(isolate,
            "JSON.stringify({value:[1,2,3].map(x=>x*x).reduce((a,b)=>a+b,0),"
            "text:'\\u4e2d\\u6587',wasm:WebAssembly.validate(new Uint8Array([0,97,115,109,1,0,0,0]))})");
        v8::Local<v8::Script> script;
        v8::Local<v8::Value> result;
        if (v8::Script::Compile(context, source).ToLocal(&script) && script->Run(context).ToLocal(&result)) {
            v8::String::Utf8Value text(isolate, result);
            std::printf("V8 %s: %s\n", v8::V8::GetVersion(), *text);
            passed = std::strcmp(*text, "{\"value\":14,\"text\":\"\xe4\xb8\xad\xe6\x96\x87\",\"wasm\":true}") == 0;
        }
    }
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    VirtualFree(code, 0, MEM_RELEASE);
    return passed ? 0 : 5;
}
