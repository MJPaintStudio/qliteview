import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 1100
    height: 760
    visible: true
    title: browser.pageTitle.length ? browser.pageTitle + " — QLiteView" : "QLiteView"
    Browser {
        id: browser
        anchors.fill: parent
    }
}
