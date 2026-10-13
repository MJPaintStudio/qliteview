import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtWebChannel
import QWebKit

ColumnLayout {
    id: root
    spacing: 0
    readonly property string pageTitle: web.title
    property string statusText: "LiteView · WebGL / media disabled"

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 8
        Button { text: "←"; enabled: web.canGoBack; onClicked: web.goBack() }
        Button { text: "→"; enabled: web.canGoForward; onClicked: web.goForward() }
        Button { text: web.loading ? "停止" : "刷新"; onClicked: web.loading ? web.stop() : web.reload() }
        TextField {
            id: address
            Layout.fillWidth: true
            text: web.url.toString()
            selectByMouse: true
            onAccepted: web.url = text.indexOf(":") > 0 ? text : "https://" + text
        }
        Button { text: "打开"; onClicked: address.accepted() }
        Button { text: "本地示例"; onClicked: web.url = Qt.resolvedUrl("pages/index.html") }
        Button { text: "JS"; onClicked: web.runJavaScript("({title:document.title, sum:6*7})", value => root.statusText = JSON.stringify(value)) }
        Button { text: "保存"; onClicked: saveDialog.open() }
    }
    ProgressBar {
        Layout.fillWidth: true
        Layout.preferredHeight: 3
        from: 0
        to: 100
        value: web.loadProgress
        visible: web.loading
    }
    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        WebEngineView {
            id: web
            objectName: "webView"
            anchors.fill: parent
            webChannel: channel
            url: Qt.resolvedUrl("pages/index.html")
            settings.javascriptEnabled: true
            settings.localStorageEnabled: true
            onLoadingChanged: request => {
                if (request.status === WebEngineView.LoadFailedStatus)
                    root.statusText = request.errorString
                else if (request.status === WebEngineView.LoadSucceededStatus)
                    root.statusText = "页面加载完成"
            }
            onNavigationRequested: request => {
                if (request.url.toString().indexOf("qwebkit-blocked:") === 0) {
                    request.reject()
                    root.statusText = "已通过 request.reject() 拦截导航"
                }
            }
            onContextMenuRequested: request => {
                request.accepted = true
                contextMenu.popup(request.position.x, request.position.y)
            }
            onJavaScriptConsoleMessage: (level, message, line, source) => console.log("LiteView:", message)
            onHtmlSaved: (destination, success) => root.statusText = success ? "已保存 " + destination : "保存失败"
        }
        Label {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 14
            text: "QML 覆盖层"
            padding: 8
            color: "#2563eb"
        }
        Menu {
            id: contextMenu
            MenuItem { text: "后退"; enabled: web.action(WebEngineView.Back).enabled; onTriggered: web.action(WebEngineView.Back).trigger() }
            MenuItem { text: "前进"; enabled: web.action(WebEngineView.Forward).enabled; onTriggered: web.action(WebEngineView.Forward).trigger() }
            MenuItem { text: "刷新"; onTriggered: web.reload() }
            MenuSeparator {}
            MenuItem { text: "复制"; onTriggered: web.action(WebEngineView.Copy).trigger() }
            MenuItem { text: "粘贴"; onTriggered: web.action(WebEngineView.Paste).trigger() }
            MenuItem { text: "保存 HTML"; onTriggered: saveDialog.open() }
            MenuItem { text: qsTr("检查器（暂不支持）"); enabled: false }
        }
    }
    Label {
        Layout.fillWidth: true
        Layout.margins: 8
        text: root.statusText
        elide: Text.ElideRight
    }
    QtObject {
        id: bridge
        WebChannel.id: "hsjpro"
        function echo(message: string): string {
            root.statusText = "WebChannel 收到：" + message
            return "Qt 收到：" + message
        }
    }
    WebChannel { id: channel; registeredObjects: [bridge] }
    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["HTML (*.html)"]
        defaultSuffix: "html"
        onAccepted: web.saveHtml(selectedFile)
    }
}
