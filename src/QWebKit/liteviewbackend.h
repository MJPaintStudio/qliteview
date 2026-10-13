#pragma once
#include <QObject>
#include <QImage>
#include <QPointer>
#include <QWebChannelAbstractTransport>
#include <functional>
#include <memory>

class WebEngineView;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QHoverEvent;
class WebEngineAction;

class LiteViewBackend : public QWebChannelAbstractTransport {
    Q_OBJECT
public:
    explicit LiteViewBackend(WebEngineView* view);
    ~LiteViewBackend() override;
    void load(const QUrl& url);
    void loadHtml(const QString& html, const QUrl& base);
    void evaluate(const QString& script, std::function<void(QVariant)> callback = {});
    void sendMessage(const QJsonObject& message) override;
    void updateSettings();
    void updateChannel();
    void updateGeometry();
    void setZoom(double zoom);
    void setBackground(const QColor& color);
    void navigate(int action);
    void validateAction(int action, WebEngineAction* target);
    void inspect();
    void mouse(QMouseEvent* event);
    void hover(QHoverEvent* event);
    void wheel(QWheelEvent* event);
    void key(QKeyEvent* event);
    void commitText(const QString& text);
    void focus(bool value);
    void capture();
    bool canGoBack() const;
    bool canGoForward() const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};
