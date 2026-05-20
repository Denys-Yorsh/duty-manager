import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCharts

/**
 * @file StatisticsView.qml
 * @brief Повна реалізація вкладки «Статистика» з картками, графіками та рейтингом.
 * Відповідає макету на 100% (пастельна палітра, кругова діаграма, таблиця рейтингу).
 */
Page {
    id: root

    // --- Властивості даних ---
    property var statsData: []
    property var chartData: []
    property int totalPersonnel: 0

    property int currentMonth: new Date().getMonth() + 1
    property int currentYear: new Date().getFullYear()
    property bool isYearly: false

    readonly property var months: ["Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень",
                                "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"]

    // Кольорова палітра (рожево-бежеві пастельні тони та корпоративний помаранчевий)
    readonly property color cardBgColor: "white" // Змінено на білий для відповідності третій картці
    readonly property color tableHeaderBg: "#FDF5E6" // Старе мереживо (бежевий)
    readonly property color accentColor: "#F39200" // Помаранчевий для акцентів

    // --- Логіка ---

    /**
     * @function formatShortName
     * @brief Скорочує ПІБ до формату «Прізвище І. П.».
     * @param fullName Повне ім'я.
     * @return Скорочене ім'я.
     */
    function formatShortName(fullName) {
        if (!fullName) return "";
        var parts = fullName.split(" ");
        if (parts.length < 2) return fullName;
        var res = parts[0] + " " + parts[1][0] + ".";
        if (parts.length >= 3) res += " " + parts[2][0] + ".";
        return res;
    }

    /**
     * @function calculateTotalChartValue
     * @brief Обчислює загальну суму значень для діаграми для розрахунку відсотків.
     */
    function calculateTotalChartValue() {
        var total = 0;
        for (var i = 0; i < chartData.length; i++) total += chartData[i].value;
        return total;
    }

    /**
     * @function openShowStatsDialog
     * @brief Відкриває діалог вибору періоду статистики.
     */
    function openShowStatsDialog() {
        yearsModel.clear()
        var years = StatisticsController.getAvailableYears()
        for (var i = 0; i < years.length; i++) yearsModel.append({year: years[i]})
        showStatsDialog.open()
    }

    /**
     * @function exportExcel
     * @brief Відкриває діалог для експорту в Excel.
     */
    function exportExcel() { excelSaveDialog.open() }

    /**
     * @function exportPdf
     * @brief Відкриває діалог для експорту в PDF.
     */
    function exportPdf() { pdfSaveDialog.open() }

    /**
     * @function refreshData
     * @brief Оновлює дані статистики та графіку з контролера.
     */
    function refreshData() {
        // Отримання основної статистики (рейтинг)
        statsData = StatisticsController.getStatistics(root.currentMonth, root.currentYear, root.isYearly)

        // Отримання розподілу для діаграми
        chartData = StatisticsController.getRankDistribution(root.currentMonth, root.currentYear, root.isYearly)

        // Підрахунок унікальних осіб
        totalPersonnel = statsData.length

        // Оновлення серій діаграми
        pieSeries.clear()
        // Змінні для пошуку найбільшого сектора
        var maxVal = -1
        var maxIndex = -1

        for (var i = 0; i < chartData.length; i++) {
            // Беремо повне звання без будь-яких скорочень
            var fullRank = chartData[i].label

            // Передаємо повну назву звання першим параметром
            var slice = pieSeries.append(fullRank, chartData[i].value)

            slice.labelVisible = true
            slice.label = slice.label

            // Напрямок тексту від зовнішнього краю до центру (радіально, як промені)
            slice.labelPosition = PieSlice.LabelInsideNormal
            slice.labelColor = "#F39200"

            // Налаштування шрифту для повних звань (8px жирний, щоб довгі слова влізли)
            //slice.labelFont.pixelSize = 8
            slice.labelFont.bold = true

            // АЛГОРИТМ ПОШУКУ НАЙБІЛЬШОГО СЕКТОРА:
            // Зберігаємо індекс сектора з максимальною кількістю нарядів
            if (chartData[i].value > maxVal) {
                maxVal = chartData[i].value
                maxIndex = i
            }
        }

        // Якщо знайшли найбільший сектор, плавно висуваємо його назовні
        if (maxIndex !== -1 && pieSeries.count > 0) {
            pieSeries.at(maxIndex).exploded = true
            pieSeries.at(maxIndex).explodeDistanceFactor = 0.08 // Коефіцієнт зсуву (оптимально 0.08)
        }
    }

    Component.onCompleted: refreshData()

    /**
     * @connection StatisticsController
     * @brief Обробка завершення експорту.
     */
    Connections {
        target: StatisticsController
        function onExportFinished(success, message) {
            exportResultText.text = message
            exportResultDialog.isError = !success
            exportResultDialog.open()
        }
    }

    // --- Компоненти дизайну ---

    /**
     * @component DialogButton
     * @brief Уніфікована кнопка для діалогів з ефектом ховеру.
     */
    component DialogButton: Button {
        id: dbBtn
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        hoverEnabled: enabled
        background: Rectangle {
            radius: 8
            color: !dbBtn.enabled ? "white" : (dbBtn.hovered ? "#F39200" : "white")
            border.color: !dbBtn.enabled ? "#E0E0E0" : (dbBtn.hovered ? "#F39200" : "#BDC3C7")
            border.width: 1
        }
        contentItem: Text {
            text: dbBtn.text
            font.bold: true
            color: !dbBtn.enabled ? "#BDC3C7" : (dbBtn.hovered ? "white" : "#2C3E50")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // --- Верстка інтерфейсу ---
    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        /**
         * @section LeftColumn
         * @brief Ліва колонка з карткою кількості та графіком.
         */
        ColumnLayout {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.35
            spacing: 20

            // КАРТКА 1: Загальна кількість
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                color: root.cardBgColor
                radius: 10
                border.color: "#DCDCDC"
                clip: true // 1. ДОБАВЛЕНО: Запрещает любому контенту физически вылезать за округлые границы карточки

                ColumnLayout {
                    // 2. ИЗМЕНЕНО: Привязываем компоновщик жестко ко ВСЕМ четырем сторонам карточки
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4 // Уменьшили отступы между текстами, чтобы они занимали меньше места по вертикали

                    Text {
                        text: root.isYearly ? "Всього військовослужбовців за " + root.currentYear + " рік"
                                          : "Всього військовослужбовців за " + root.months[root.currentMonth-1] + " " + root.currentYear
                        font.pixelSize: 16
                        font.bold: true
                        color: "#4A4A4A"
                        Layout.fillWidth: true
                        Layout.fillHeight: true // 3. ДОБАВЛЕНО: Дает право QML динамически распределять высоту
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter // Центрируем текст внутри выделенного ему пространства
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        text: root.totalPersonnel
                        font.pixelSize: 64
                        font.bold: true
                        color: root.accentColor
                        Layout.fillWidth: true
                        Layout.fillHeight: true // 4. ДОБАВЛЕНО: Позволяет тексту уменьшаться по высоте
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        fontSizeMode: Text.Fit // Автоматически уменьшает шрифт цифры, если сжимается карточка
                        minimumPixelSize: 20   // Минимальный порог шрифта, ниже которого цифра не сожмется
                    }

                    Text {
                        text: "Кількість осіб, залучених до нарядів"
                        font.pixelSize: 14
                        color: "#7F8C8D"
                        Layout.fillWidth: true
                        Layout.fillHeight: true // 5. ДОБАВЛЕНО: Дает адаптивность нижнему подпису
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // КАРТКА 2: Графік (Кругова діаграма)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: root.cardBgColor
                radius: 10
                border.color: "#DCDCDC"
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 0 // Зменшуємо відстань між заголовком та графіком

                    Text {
                        text: root.isYearly ? "Графік нарядів за " + root.currentYear + " рік"
                                          : "Графік нарядів за " + root.months[root.currentMonth-1] + " " + root.currentYear
                        font.pixelSize: 16
                        font.bold: true
                        color: "#4A4A4A"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 0 // СТРОГО ОБНУЛЯЕМ: чтобы не толкать график вниз
                    }

                    // Область діаграми
                    ChartView {
                        id: chartView
                        Layout.fillWidth: true
                        // ЗБІЛЬШЕНО: тепер займає 75% висоти картки замість 50%
                        Layout.preferredHeight: parent.height * 0.75
                        backgroundColor: "transparent"
                        legend.visible: false
                        antialiasing: true

                        // КРИТИЧЕСКИЙ МОМЕНТ: Отрицательный отступ подтянет график вверх к тексту
                        Layout.topMargin: -10 // Если нужно еще выше, поставь -30 или -35
                        Layout.bottomMargin: 0

                        // Налаштування відступів для максимального заповнення простору
                        margins.top: 0
                        margins.bottom: 0
                        margins.left: 0
                        margins.right: 0

                        PieSeries {
                            id: pieSeries
                            holeSize: 0.0
                            // ДОДАНО: Розмір серії (0.9 - майже на всю доступну область ChartView)
                            size: 1
                        }
                    }

                    // Кастомна легенда з відсотками під діаграмою
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        // Додамо невеликий відступ зверху, щоб легенда не "липла" до графіка
                        Layout.topMargin: 0

                        ColumnLayout {
                            width: parent.width
                            spacing: 5

                            Repeater {
                                model: root.chartData
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    Rectangle {
                                        width: 12; height: 12
                                        radius: 6
                                        color: pieSeries.at(index) ? pieSeries.at(index).color : "gray"
                                    }

                                    Text {
                                        text: modelData.label
                                        font.pixelSize: 13
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        property real total: root.calculateTotalChartValue()
                                        text: (total > 0 ? ((modelData.value / total) * 100).toFixed(1) : "0") + "%"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: root.accentColor
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /**
         * @section RightColumn
         * @brief Права колонка з таблицею рейтингу.
         */
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: 400
            color: "white"
            radius: 10
            border.color: "#DCDCDC"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                Text {
                    text: "Рейтинг за кількість днів наряду:"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#2C3E50"
                }

                // Шапка таблиці
                Rectangle {
                    Layout.fillWidth: true
                    height: 45
                    color: root.tableHeaderBg
                    radius: 5

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        spacing: 0

                        Text { Layout.preferredWidth: parent.width * 0.5; text: "ПІБ"; font.bold: true; color: "#34495E" }
                        Text { Layout.preferredWidth: parent.width * 0.25; text: "Звання"; font.bold: true; color: "#34495E" }
                        Text { Layout.fillWidth: true; text: "Кількість днів наряду"; font.bold: true; color: "#34495E"; horizontalAlignment: Text.AlignRight }
                    }
                }

                // Список рейтингу
                ListView {
                    id: ratingList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.statsData
                    spacing: 2

                    delegate: Rectangle {
                        width: ratingList.width
                        height: 40
                        color: index % 2 === 0 ? "#FAFAFA" : "white"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            spacing: 0

                            Text {
                                Layout.preferredWidth: parent.width * 0.5
                                text: modelData.index + ". " + root.formatShortName(modelData.fullName)
                                font.pixelSize: 14
                                color: "#2C3E50"
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.preferredWidth: parent.width * 0.25
                                text: modelData.rankName
                                font.pixelSize: 14
                                color: "#7F8C8D"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.dutyCount
                                font.pixelSize: 14
                                font.bold: true
                                color: root.accentColor
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                }
            }
        }
    }

    // --- Модальні вікна ---

    Dialog {
        id: showStatsDialog; width: 400; padding: 25; modal: true
        x: Math.round((parent.width - width) / 2); y: Math.round((parent.height - height) / 2)
        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc" }
        header: Label { text: "Відобразити статистику"; font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter }

        contentItem: ColumnLayout {
            spacing: 20

            ComboBox {
                id: periodCombo
                Layout.fillWidth: true
                model: ["за місяць", "за рік"]
                currentIndex: root.isYearly ? 1 : 0
            }

            ComboBox {
                id: yearStatsCombo
                Layout.fillWidth: true
                textRole: "year"
                model: yearsModel
                onActivated: {
                    monthsStatsModel.clear()
                    if (periodCombo.currentIndex === 0) {
                        var ms = StatisticsController.getAvailableMonths(parseInt(currentText))
                        for (var i = 0; i < ms.length; i++) monthsStatsModel.append({mIdx: ms[i], mName: root.months[ms[i]-1]})
                    }
                }
            }

            ComboBox {
                id: monthStatsCombo
                Layout.fillWidth: true
                enabled: periodCombo.currentIndex === 0
                textRole: "mName"
                model: monthsStatsModel
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 15
                DialogButton { text: "Відміна"; onClicked: showStatsDialog.close() }
                DialogButton {
                    text: "Примінити"
                    onClicked: {
                        root.isYearly = (periodCombo.currentIndex === 1)
                        if (yearStatsCombo.currentIndex !== -1) {
                            root.currentYear = parseInt(yearStatsCombo.currentText)
                        }
                        if (!root.isYearly) {
                            var mIdx = monthStatsCombo.currentIndex
                            if (mIdx !== -1) {
                                root.currentMonth = monthsStatsModel.get(mIdx).mIdx
                            }
                        }
                        refreshData()
                        showStatsDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: exportResultDialog; width: 400; padding: 25; modal: true; property bool isError: false
        x: Math.round((parent.width - width) / 2); y: Math.round((parent.height - height) / 2)
        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc" }
        header: Label {
            text: exportResultDialog.isError ? "Помилка" : "Успіх"
            font.pixelSize: 22; font.bold: true; color: exportResultDialog.isError ? "#E74C3C" : "black"; padding: 20; horizontalAlignment: Text.AlignHCenter
        }
        contentItem: ColumnLayout {
            spacing: 25
            Text { id: exportResultText; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            DialogButton { text: "Зрозуміло"; onClicked: exportResultDialog.close() }
        }
    }

    ListModel { id: yearsModel }
    ListModel { id: monthsStatsModel }

    // --- Діалоги збереження ---
    FileDialog {
        id: excelSaveDialog; title: "Зберегти як Excel"; fileMode: FileDialog.SaveFile
        nameFilters: ["Excel files (*.xlsx)"]
        onAccepted: StatisticsController.exportToExcel(root.statsData, selectedFile.toString())
    }

    FileDialog {
        id: pdfSaveDialog; title: "Зберегти як PDF"; fileMode: FileDialog.SaveFile
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: StatisticsController.exportToPdf(root.statsData, selectedFile.toString())
    }
}
