import "Common.js" as Common
import EEdbm
import EEpw
import EEtagsPluginName
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard

Kirigami.ScrollablePage {
    id: limiterPage

    required property var name
    required property var pluginDB
    required property var pipelineInstance
    property var pluginBackend

    function updateMeters() {
        if (!pluginBackend)
            return ;

        inputOutputLevels.inputLevelLeft = pluginBackend.getInputLevelLeft();
        inputOutputLevels.inputLevelRight = pluginBackend.getInputLevelRight();
        inputOutputLevels.outputLevelLeft = pluginBackend.getOutputLevelLeft();
        inputOutputLevels.outputLevelRight = pluginBackend.getOutputLevelRight();
        gainLevelLeft.value = pluginBackend.getGainLevelLeft();
        gainLevelRight.value = pluginBackend.getGainLevelRight();
        sideChainLevelLeft.value = pluginBackend.getSideChainLevelLeft();
        sideChainLevelRight.value = pluginBackend.getSideChainLevelRight();
    }

    Component.onCompleted: {
        pluginBackend = pipelineInstance.getPluginInstance(name);
    }

    ColumnLayout {
        Kirigami.CardsLayout {
            id: cardLayout

            maximumColumns: 4
            uniformCellWidths: true

            Kirigami.Card {
                id: cardMode

                contentItem: Column {
                    FormCard.FormComboBoxDelegate {
                        id: mode

                        text: i18n("Mode")
                        displayMode: FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: pluginDB.mode
                        editable: false
                        model: [i18n("Herm Thin"), i18n("Herm Wide"), i18n("Herm Tail"), i18n("Herm Duck"), i18n("Exp Thin"), i18n("Exp Wide"), i18n("Exp Tail"), i18n("Exp Duck"), i18n("Line Thin"), i18n("Line Wide"), i18n("Line Tail"), i18n("Line Duck")]
                        onActivated: (idx) => {
                            pluginDB.mode = idx;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    FormCard.FormComboBoxDelegate {
                        id: oversampling

                        text: i18n("Oversampling")
                        displayMode: FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: pluginDB.oversampling
                        editable: false
                        model: [i18n("None"), i18n("Half x2/16 bit"), i18n("Half x2/24 bit"), i18n("Half x3/16 bit"), i18n("Half x3/24 bit"), i18n("Half x4/16 bit"), i18n("Half x4/24 bit"), i18n("Half x6/16 bit"), i18n("Half x6/24 bit"), i18n("Half x8/16 bit"), i18n("Half x8/24 bit"), i18n("Full x2/16 bit"), i18n("Full x2/24 bit"), i18n("Full x3/16 bit"), i18n("Full x3/24 bit"), i18n("Full x4/16 bit"), i18n("Full x4/24 bit"), i18n("Full x6/16 bit"), i18n("Full x6/24 bit"), i18n("Full x8/16 bit"), i18n("Full x8/24 bit")]
                        onActivated: (idx) => {
                            pluginDB.oversampling = idx;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    FormCard.FormComboBoxDelegate {
                        id: dithering

                        text: i18n("Dithering")
                        displayMode: FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: pluginDB.dithering
                        editable: false
                        model: [i18n("None"), i18n("7 bit"), i18n("8 bit"), i18n("11 bit"), i18n("12 bit"), i18n("15 bit"), i18n("16 bit"), i18n("23 bit"), i18n("24 bit")]
                        onActivated: (idx) => {
                            pluginDB.dithering = idx;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                }

            }

            Kirigami.Card {
                id: cardLimiter

                header: Kirigami.Heading {
                    text: i18n("Limiter")
                    level: 2
                }

                contentItem: Column {
                    EeSpinBox {
                        id: threshold

                        label: i18n("Threshold")
                        from: pluginDB.getMinValue("threshold")
                        to: pluginDB.getMaxValue("threshold")
                        value: pluginDB.threshold
                        decimals: 2
                        stepSize: 0.01
                        unit: "dB"
                        onValueModified: (v) => {
                            pluginDB.threshold = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    EeSpinBox {
                        id: attack

                        label: i18n("Attack")
                        from: pluginDB.getMinValue("attack")
                        to: pluginDB.getMaxValue("attack")
                        value: pluginDB.attack
                        decimals: 2
                        stepSize: 0.01
                        unit: "ms"
                        onValueModified: (v) => {
                            pluginDB.attack = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    EeSpinBox {
                        id: release

                        label: i18n("Release")
                        from: pluginDB.getMinValue("release")
                        to: pluginDB.getMaxValue("release")
                        value: pluginDB.release
                        decimals: 2
                        stepSize: 0.01
                        unit: "ms"
                        onValueModified: (v) => {
                            pluginDB.release = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    EeSpinBox {
                        id: stereoLink

                        label: i18n("Stereo Link")
                        from: pluginDB.getMinValue("stereoLink")
                        to: pluginDB.getMaxValue("stereoLink")
                        value: pluginDB.stereoLink
                        decimals: 1
                        stepSize: 0.1
                        unit: "%"
                        onValueModified: (v) => {
                            pluginDB.stereoLink = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                }

            }

            Kirigami.Card {
                id: cardSideChain

                header: Kirigami.Heading {
                    text: i18n("Sidechain")
                    level: 2
                }

                contentItem: Column {
                    id: cardSideChainColumn

                    FormCard.FormComboBoxDelegate {
                        id: sidechainType

                        text: i18n("Type")
                        displayMode: FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: pluginDB.sidechainType
                        editable: false
                        model: [i18n("Internal"), i18n("External"), i18n("Link")]
                        onActivated: (idx) => {
                            pluginDB.sidechainType = idx;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    FormCard.FormComboBoxDelegate {
                        id: comboSideChainInputDevice

                        text: i18n("Input Device")
                        displayMode: FormCard.FormComboBoxDelegate.ComboBox
                        editable: false
                        model: ModelNodes
                        textRole: "description"
                        enabled: sidechainType.currentIndex === 1
                        currentIndex: {
                            for (let n = 0; n < ModelNodes.rowCount(); n++) {
                                if (ModelNodes.getNodeName(n) === pluginDB.sidechainInputDevice)
                                    return n;

                            }
                            return 0;
                        }
                        onActivated: (idx) => {
                            let selectedName = ModelNodes.getNodeName(idx);
                            if (selectedName !== pluginDB.sidechainInputDevice)
                                pluginDB.sidechainInputDevice = selectedName;

                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    EeSpinBox {
                        id: sidechainPreamp

                        label: i18n("Preamp")
                        from: pluginDB.getMinValue("sidechainPreamp")
                        to: pluginDB.getMaxValue("sidechainPreamp")
                        value: pluginDB.sidechainPreamp
                        decimals: 2
                        stepSize: 0.01
                        unit: "dB"
                        minusInfinityMode: true
                        onValueModified: (v) => {
                            pluginDB.sidechainPreamp = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                }

            }

            Kirigami.Card {
                id: cardALR

                enabled: pluginDB.alr

                header: Kirigami.Heading {
                    text: i18n("Automatic Level")
                    level: 2
                }

                contentItem: Column {
                    EeSpinBox {
                        id: alrAttack

                        label: i18n("Attack")
                        from: pluginDB.getMinValue("alrAttack")
                        to: pluginDB.getMaxValue("alrAttack")
                        value: pluginDB.alrAttack
                        decimals: 2
                        stepSize: 0.01
                        unit: "ms"
                        onValueModified: (v) => {
                            pluginDB.alrAttack = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    EeSpinBox {
                        id: alrRelease

                        label: i18n("Release")
                        from: pluginDB.getMinValue("alrRelease")
                        to: pluginDB.getMaxValue("alrRelease")
                        value: pluginDB.alrRelease
                        decimals: 1
                        stepSize: 0.1
                        unit: "ms"
                        onValueModified: (v) => {
                            pluginDB.alrRelease = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                    EeSpinBox {
                        id: alrKnee

                        label: i18n("Knee")
                        from: pluginDB.getMinValue("alrKnee")
                        to: pluginDB.getMaxValue("alrKnee")
                        value: pluginDB.alrKnee
                        decimals: 2
                        stepSize: 0.01
                        unit: "dB"
                        onValueModified: (v) => {
                            pluginDB.alrKnee = v;
                        }

                        anchors {
                            left: parent.left
                            right: parent.right
                        }

                    }

                }

            }

        }

        Kirigami.Card {
            id: cardLevels

            Layout.fillWidth: false
            Layout.alignment: Qt.AlignHCenter

            contentItem: GridLayout {
                id: levelGridLayout

                readonly property real radius: 2.5 * Kirigami.Units.gridUnit

                columnSpacing: Kirigami.Units.largeSpacing
                rowSpacing: Kirigami.Units.largeSpacing
                columns: 4
                rows: 3

                Controls.Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("Gain")
                }

                Controls.Label {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.gridUnit
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("Sidechain")
                }

                EeCircularProgress {
                    id: gainLevelLeft

                    Layout.alignment: Qt.AlignBottom
                    implicitWidth: levelGridLayout.radius
                    implicitHeight: levelGridLayout.radius
                    from: Common.minimumDecibelLevel
                    to: 0
                    value: 0
                    decimals: 0
                    convertDecibelToLinear: true
                }

                EeCircularProgress {
                    id: gainLevelRight

                    Layout.alignment: Qt.AlignBottom
                    implicitWidth: levelGridLayout.radius
                    implicitHeight: levelGridLayout.radius
                    from: Common.minimumDecibelLevel
                    to: 0
                    value: 0
                    decimals: 0
                    convertDecibelToLinear: true
                }

                EeCircularProgress {
                    id: sideChainLevelLeft

                    Layout.alignment: Qt.AlignBottom
                    Layout.leftMargin: Kirigami.Units.gridUnit
                    implicitWidth: levelGridLayout.radius
                    implicitHeight: levelGridLayout.radius
                    from: Common.minimumDecibelLevel
                    to: 0
                    value: 0
                    decimals: 0
                    convertDecibelToLinear: true
                }

                EeCircularProgress {
                    id: sideChainLevelRight

                    Layout.alignment: Qt.AlignBottom
                    implicitWidth: levelGridLayout.radius
                    implicitHeight: levelGridLayout.radius
                    from: Common.minimumDecibelLevel
                    to: 0
                    value: 0
                    decimals: 0
                    convertDecibelToLinear: true
                }

                Controls.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("L")
                }

                Controls.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("R")
                }

                Controls.Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.gridUnit
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("L")
                }

                Controls.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: i18n("R")
                }

            }

        }

    }

    header: EeInputOutputGain {
        id: inputOutputLevels

        pluginDB: limiterPage.pluginDB
    }

    footer: RowLayout {
        Controls.Label {
            text: i18n("Using") + EEtagsPluginPackage.lsp
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            Layout.fillWidth: false
            Layout.leftMargin: Kirigami.Units.smallSpacing
            color: Kirigami.Theme.disabledTextColor
        }

        Kirigami.ActionToolBar {
            Layout.margins: Kirigami.Units.smallSpacing
            alignment: Qt.AlignRight
            position: Controls.ToolBar.Footer
            flat: true
            actions: [
                Kirigami.Action {
                    text: i18n("Show Native Window")
                    icon.name: "window-duplicate-symbolic"
                    enabled: EEdbm.main.showNativePluginUi
                    checkable: true
                    checked: pluginBackend.hasNativeUi()
                    onTriggered: {
                        if (checked)
                            pluginBackend.show_native_ui();
                        else
                            pluginBackend.close_native_ui();
                    }
                },
                Kirigami.Action {
                    text: i18n("Gain Boost")
                    icon.name: "usermenu-up-symbolic"
                    checkable: true
                    checked: pluginDB.gainBoost
                    onTriggered: {
                        if (pluginDB.gainBoost != checked)
                            pluginDB.gainBoost = checked;

                    }
                },
                Kirigami.Action {
                    text: i18n("Automatic Level")
                    icon.name: "usermenu-up-symbolic"
                    checkable: true
                    checked: pluginDB.alr
                    onTriggered: {
                        if (pluginDB.alr != checked)
                            pluginDB.alr = checked;

                    }
                },
                Kirigami.Action {
                    text: i18n("Reset")
                    icon.name: "edit-reset-symbolic"
                    onTriggered: {
                        pluginBackend.reset();
                    }
                }
            ]
        }

    }

}
