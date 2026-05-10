import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Sinophone.Qt 1.0

// Drop-in download UI for sinophone-family apps.
// Required: set the `downloader` property to a ModelDownloader instance.
//
// Color properties can be overridden to match the host app's theme.
Item {
    id: root

    required property ModelDownloader downloader

    // Theming — override to match host app
    property color colorAccent:     "#7b9ac4"
    property color colorSurface:    "#2a2a3e"
    property color colorSurface2:   "#3a3a5e"
    property color colorText:       "#e0e0e0"
    property color colorTextDim:    "#888899"
    property color colorSuccess:    "#4caf50"
    property color colorError:      "#cf6679"

    implicitHeight: col.implicitHeight
    implicitWidth:  col.implicitWidth

    // ── internal state ────────────────────────────────────────────────────
    property string _selectedModel: "Qwen2.5-7B-Q4"
    property string _statusOverride: ""   // success/error message after finish

    Connections {
        target: root.downloader
        function onFinished(success, path, error) {
            if (success) {
                root._statusOverride = "Saved: " + path.split("/").pop()
            } else {
                root._statusOverride = "Error: " + error
            }
        }
    }

    ColumnLayout {
        id: col
        anchors.left:  parent.left
        anchors.right: parent.right
        spacing: 10

        // ── model size selector ──────────────────────────────────────────
        Label {
            text: "Download a model"
            color: root.colorTextDim
            font.pixelSize: 13
        }

        Label {
            text: "Models saved to: " + ModelDownloader.sharedModelsDir()
            color: root.colorTextDim
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 6

            Repeater {
                model: [
                    ["Qwen2.5-1.5B-Q4", "1.5 B"],
                    ["Qwen2.5-3B-Q4",   "3 B"  ],
                    ["Qwen2.5-7B-Q4",   "7 B"  ],
                    ["Qwen2.5-14B-Q4",  "14 B" ],
                ]
                delegate: Rectangle {
                    required property var modelData
                    width: 62; height: 34; radius: 6
                    color:        root._selectedModel === modelData[0] ? root.colorSurface2 : root.colorSurface
                    border.color: root._selectedModel === modelData[0] ? root.colorAccent   : "transparent"
                    border.width: 1
                    Label {
                        anchors.centerIn: parent
                        text:  modelData[1]
                        color: root.colorText
                        font.pixelSize: 13
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root._selectedModel   = modelData[0]
                            root._statusOverride  = ""
                        }
                    }
                }
            }
        }

        // ── download / cancel row ────────────────────────────────────────
        RowLayout {
            spacing: 8

            Rectangle {
                width: 110; height: 34; radius: 6
                color:   root.downloader.busy ? root.colorSurface : root.colorAccent
                opacity: root.downloader.busy ? 0.6 : 1.0
                Label {
                    anchors.centerIn: parent
                    text:  root.downloader.busy ? "Downloading…" : "Download"
                    color: root.colorText
                    font.pixelSize: 13
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: !root.downloader.busy
                    onClicked: {
                        root._statusOverride = ""
                        root.downloader.download(root._selectedModel)
                    }
                }
            }

            Rectangle {
                visible: root.downloader.busy
                width: 72; height: 34; radius: 6
                color: root.colorSurface
                Label {
                    anchors.centerIn: parent
                    text:  "Cancel"
                    color: root.colorAccent
                    font.pixelSize: 13
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.downloader.cancel()
                }
            }
        }

        // ── progress bar ─────────────────────────────────────────────────
        ProgressBar {
            Layout.fillWidth: true
            visible: root.downloader.busy
            value:   root.downloader.progress
            background: Rectangle {
                implicitHeight: 8; radius: 4
                color: root.colorSurface
            }
            contentItem: Rectangle {
                width:  parent.visualPosition * parent.width
                height: parent.height
                radius: 4
                color:  root.colorAccent
            }
        }

        // ── status / error text ───────────────────────────────────────────
        Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: root._statusOverride !== ""
                  ? root._statusOverride
                  : root.downloader.statusText
            color: root._statusOverride.startsWith("Error")
                   ? root.colorError
                   : root._statusOverride !== "" ? root.colorSuccess : root.colorTextDim
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
