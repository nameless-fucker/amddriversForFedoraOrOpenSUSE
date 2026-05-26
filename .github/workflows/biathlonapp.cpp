#include "biathlonapp.h"
#include <QVBoxLayout>     // вертикальное расположение элементов
#include <QHBoxLayout>     // горизонтальное расположение кнопок
#include <QHeaderView>     // настройка заголовков таблицы
#include <QFont>           // работа со шрифтами (для призёров)
#include <QMessageBox>     // всплывающие сообщения об ошибках
#include <algorithm>       // std::sort для сортировки результатов
#include <chrono>          // std::chrono для инициализации ГСЧ

// Конструктор — создаёт окно, кнопки, таблицу, загружает спортсменов
BiathlonApp::BiathlonApp(QWidget *parent)
    : QWidget(parent),
      rng(std::chrono::steady_clock::now().time_since_epoch().count())
{
    setWindowTitle("Биатлон: симулятор гонки");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Выпадающий список с 4 дисциплинами
    raceTypeCombo = new QComboBox(this);
    raceTypeCombo->addItem("Спринт (10 км)");
    raceTypeCombo->addItem("Гонка преследования (12.5 км)");
    raceTypeCombo->addItem("Индивидуальная гонка (20 км)");
    raceTypeCombo->addItem("Масс-старт (15 км)");

    mainLayout->addWidget(new QLabel("Выберите дисциплину:", this));
    mainLayout->addWidget(raceTypeCombo);

    // Кнопки управления
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnGenerate = new QPushButton("1. Жеребьёвка", this);
    btnStart    = new QPushButton("2. Старт гонки", this);
    btnResults  = new QPushButton("3. Показать результаты", this);

    QString btnStyle = "QPushButton {"
        "background-color: #4a90d9; color: white; border: none;"
        "padding: 8px 16px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #357abd; }";
    btnGenerate->setStyleSheet(btnStyle);
    btnStart->setStyleSheet(btnStyle);
    btnResults->setStyleSheet(btnStyle);

    btnLayout->addWidget(btnGenerate);
    btnLayout->addWidget(btnStart);
    btnLayout->addWidget(btnResults);
    mainLayout->addLayout(btnLayout);

    // Вкладки: Гонка и Профиль участника
    mainTabs = new QTabWidget(this);

    // ---- Вкладка "Гонка" ----
    QWidget *raceTab = new QWidget();
    QVBoxLayout *raceLayout = new QVBoxLayout(raceTab);

    outputLog = new QTextEdit(this);
    outputLog->setReadOnly(true);
    outputLog->setMaximumHeight(300);
    outputLog->setStyleSheet(
        "QTextEdit { background-color: #ffffff; color: #000000;"
        "border: 1px solid #cccccc; border-radius: 5px; padding: 5px; }");

    resultsTable = new QTableWidget(this);
    resultsTable->setColumnCount(5);
    resultsTable->setHorizontalHeaderLabels(
        {"Место", "Номер", "Спортсмен", "Промахи", "Время"});
    resultsTable->horizontalHeader()->setStretchLastSection(true);
    resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTableStyle();

    raceLayout->addWidget(outputLog);
    raceLayout->addWidget(resultsTable);
    mainTabs->addTab(raceTab, "Гонка");

    // ---- Вкладка "Профиль участника" ----
    QWidget *infoTab = new QWidget();
    QVBoxLayout *infoLayout = new QVBoxLayout(infoTab);

    athleteInfoLabel = new QLabel(this);
    athleteInfoLabel->setWordWrap(true);
    athleteInfoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    athleteInfoLabel->setStyleSheet(
        "QLabel { color: #000000; background-color: #ffffff;"
        "padding: 15px; border: 1px solid #cccccc; border-radius: 5px; font-size: 13px; }");
    athleteInfoLabel->setText(
        "Выберите спортсмена в таблице для просмотра информации.");

    infoLayout->addWidget(athleteInfoLabel);
    mainTabs->addTab(infoTab, "Профиль участника");

    mainLayout->addWidget(mainTabs);

    // Подключаем сигналы кнопок к обработчикам
    connect(btnGenerate, &QPushButton::clicked,
            this, &BiathlonApp::onGenerateStartList);
    connect(btnStart, &QPushButton::clicked,
            this, &BiathlonApp::onStartRace);
    connect(btnResults, &QPushButton::clicked,
            this, &BiathlonApp::onShowResults);
    connect(resultsTable, &QTableWidget::cellClicked,
            this, &BiathlonApp::onAthleteSelected);

    initAthletes();
    currentRace = RaceType::Sprint;
    outputLog->append(
        "<b>Выберите дисциплину и нажмите «Жеребьёвка».</b>");
}

// Заполняю список биатлонистов (взял из интернета, Кубок мира 2025/2026)
void BiathlonApp::initAthletes() {
    teamPool = {
        {"Йоханнес Бё",         "Норвегия", 32, 187, 18,
         "5-кратный ОИ, 20-кратный ЧМ, 5 Кубков мира",
         "Универсал, лучшая энергоэффективность",
         19.8, 0.90, 23, 95},
        {"Стурла Холм Лагрейд", "Норвегия", 28, 182, 12,
         "ОИ 2022 (эстафета), 5-кратный ЧМ",
         "Лучшая точность в мире (98% лёжа)",
         20.2, 0.94, 26, 90},
        {"Кентен Фийон Майе",   "Франция",  33, 177, 15,
         "ОИ 2022 (преследование), КМ 2021/22",
         "Стабильный универсал, силён в контактных гонках",
         20.3, 0.87, 24, 88},
        {"Эмильен Жакелен",     "Франция",  29, 179, 10,
         "ЧМ 2025 (преследование), призёр ОИ 2022",
         "Один из быстрейших лыжников, агрессивный стиль",
         19.6, 0.84, 25, 82},
        {"Себастьян Самуэльссон","Швеция",  28, 183, 11,
         "Призёр ОИ 2018, 2-кратный ЧМ",
         "Мощный лыжный ход, финишный спринт",
         20.5, 0.85, 26, 85},
        {"Мартин Понсилуома",   "Швеция",  29, 181, 10,
         "ЧМ 2024 (масс-старт)",
         "Взрывная скорость, нестабильная стрельба стоя",
         20.7, 0.83, 24, 80},
        {"Тарьей Бё",           "Норвегия", 36, 185, 20,
         "ОИ 2010 (эстафета), 11 золотых ЧМ в эстафетах",
         "Опытный ветеран, отличная стрельба лёжа",
         20.4, 0.88, 25, 87},
        {"Ветле Ш. Кристиансен","Норвегия", 33, 184, 13,
         "ОИ 2022 (эстафета), ЧМ 2024",
         "Надёжный командный боец, стабильная стрельба",
         20.6, 0.85, 27, 84},
        {"Бенедикт Долль",      "Германия", 35, 178, 16,
         "ЧМ 2017 (спринт), призёр ОИ 2018",
         "Сильный лыжник, хорошая техника",
         20.5, 0.86, 24, 83},
        {"Владимир Илиев",      "Болгария", 38, 182, 19,
         "Многократный ЧМ Болгарии, призёр Кубка IBU",
         "Опытный снайпер, стабильный стрелок",
         21.2, 0.88, 28, 79},
        {"Дмитрий Пидручный",   "Украина",  33, 178, 14,
         "ЧМ 2019 (преследование)",
         "Крепкий универсал, лидерские качества",
         21.0, 0.83, 28, 81},
        {"Антонен Гигонна",     "Франция",  28, 175, 8,
         "Призёр КМ, ЧМ среди юниоров",
         "Хорошая скорость, перспективный",
         20.9, 0.81, 26, 77}
    };
}

// Просто делаю таблицу красивой (белый фон, голубые заголовки)
void BiathlonApp::setupTableStyle() {
    resultsTable->setStyleSheet(
        "QTableWidget {"
        "background-color: #ffffff; color: #000000;"
        "gridline-color: #cccccc; border: 1px solid #cccccc;"
        "border-radius: 5px; }"
        "QHeaderView::section {"
        "background-color: #4a90d9; color: white;"
        "font-weight: bold; padding: 6px; border: none;"
        "border-right: 1px solid #357abd; }");
    resultsTable->setAlternatingRowColors(true);
}

// Жеребьёвка — перемешиваю номера и создаю участников
void BiathlonApp::onGenerateStartList() {
    participants.clear();
    resultsTable->clearContents();
    resultsTable->setRowCount(0);
    outputLog->clear();
    outputLog->append("<b>=== ЖЕРЕБЬЁВКА ===</b>");

    // Определяем дисциплину по тексту из комбобокса
    QString selected = raceTypeCombo->currentText();
    if (selected.contains("Спринт"))
        currentRace = RaceType::Sprint;
    else if (selected.contains("Преследовани"))
        currentRace = RaceType::Pursuit;
    else if (selected.contains("Индивидуальн"))
        currentRace = RaceType::Individual;
    else if (selected.contains("Масс-старт"))
        currentRace = RaceType::MassStart;

    outputLog->append(QString("Дисциплина: <b>%1</b>").arg(selected));

    // Создаю номера 1..N и случайно перемешиваю
    std::vector<int> numbers(teamPool.size());
    for (size_t i = 0; i < numbers.size(); ++i)
        numbers[i] = static_cast<int>(i + 1);
    std::shuffle(numbers.begin(), numbers.end(), rng);

    for (size_t i = 0; i < teamPool.size(); ++i) {
        auto p = std::make_shared<RaceParticipant>(teamPool[i], numbers[i]);
        participants.push_back(p);
        outputLog->append(
            QString("Номер %1 : %2 (%3)")
                .arg(p->startNumber).arg(p->info.name).arg(p->info.country));
    }
}

// Симуляция стрельбы — 5 выстрелов, точность падает если устал и если стреляет стоя
void BiathlonApp::simShooting(RaceParticipant &p, int lapNumber, int totalLaps) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    int misses = 0;
    bool isStanding = (lapNumber % 2 == 0);  // чётный круг = стрельба стоя
    double adjustedAccuracy = p.info.shootingAccuracy;
    adjustedAccuracy *= (1.0 - (lapNumber / (double)totalLaps) * 0.08);  // устал -> хуже стреляет
    if (isStanding)
        adjustedAccuracy -= 0.06;  // стоя стрелять сложнее

    for (int shot = 0; shot < 5; ++shot) {
        if (dist(rng) > adjustedAccuracy)
            ++misses;
    }

    p.missedShots += misses;
    p.totalTime += p.info.shootingTime + misses * 12.0;  // +12 сек за каждый промах
}

// Симуляция бега — скорость * дистанция + усталость (чем меньше выносливость, тем сильнее устаёт)
double BiathlonApp::simRunning(
    const RaceParticipant &p, double distanceKm, int lapNumber, int totalLaps)
{
    std::normal_distribution<double> noise(0.0, 1.0);
    double endurancePenalty = (100.0 - p.info.endurance) / 100.0;
    double fatigue = 1.0 + (lapNumber / (double)totalLaps)
                          * endurancePenalty * 0.15;
    return p.info.runningSpeed * distanceKm * fatigue + noise(rng) * 0.8;
}

// Старт гонки — бегу по всем участникам, на каждом круге считаю время бега и стрельбу
void BiathlonApp::onStartRace() {
    if (participants.empty()) {
        QMessageBox::warning(this, "Ошибка", "Сначала выполните жеребьёвку!");
        return;
    }

    outputLog->clear();
    outputLog->append("<b>=== СТАРТ ГОНКИ ===</b>");

    double totalDistance;
    int shootingStops;

    // Выбираю параметры по типу гонки
    switch (currentRace) {
    case RaceType::Sprint:
        totalDistance = 10.0; shootingStops = 2; break;
    case RaceType::Pursuit:
        totalDistance = 12.5; shootingStops = 4; break;
    case RaceType::Individual:
        totalDistance = 20.0; shootingStops = 4; break;
    case RaceType::MassStart:
        totalDistance = 15.0; shootingStops = 4; break;
    default:
        totalDistance = 10.0; shootingStops = 2; break;
    }

    int totalLaps = shootingStops + 1;
    double distPerLap = totalDistance / totalLaps;

    outputLog->append(
        QString("Дистанция: <b>%1 км</b>, кругов: %2, рубежей: %3")
            .arg(totalDistance).arg(totalLaps).arg(shootingStops));

    // Для каждого участника пробегаем все круги, на рубежах стреляем
    for (auto &p : participants) {
        double runTime = 0.0;
        for (int lap = 0; lap < totalLaps; ++lap) {
            runTime += simRunning(*p, distPerLap, lap, totalLaps);
            if (lap < shootingStops) {
                p->totalTime += runTime;
                runTime = 0.0;
                simShooting(*p, lap, totalLaps);
            }
        }
        p->totalTime += runTime;
        p->finished = true;
    }

    outputLog->append(
        "<b>Гонка завершена! Нажмите «Показать результаты».</b>");
}

// Сортирую участников по времени — у кого меньше, тот выше в таблице
void BiathlonApp::calculateResults() {
    std::sort(participants.begin(), participants.end(),
        [](const auto &a, const auto &b) { return a->totalTime < b->totalTime; });
}

// Вывожу таблицу с результатами, первые три места — золото/серебро/бронза
void BiathlonApp::onShowResults() {
    if (participants.empty() || !participants.front()->finished) {
        QMessageBox::warning(this, "Ошибка", "Сначала проведите гонку!");
        return;
    }

    calculateResults();
    resultsTable->setRowCount(participants.size());

    QColor gold(255, 215, 0), silver(192, 192, 192), bronze(205, 127, 50);

    for (size_t i = 0; i < participants.size(); ++i) {
        auto &p = participants[i];
        int place = static_cast<int>(i + 1);

        auto *itemPlace = new QTableWidgetItem(QString::number(place));
        auto *itemNum   = new QTableWidgetItem(QString::number(p->startNumber));
        auto *itemName  = new QTableWidgetItem(
            p->info.name + " (" + p->info.country + ")");
        auto *itemMiss  = new QTableWidgetItem(QString::number(p->missedShots));
        auto *itemTime  = new QTableWidgetItem(
            QString::number(p->totalTime, 'f', 1) + " сек");

        itemPlace->setTextAlignment(Qt::AlignCenter);
        itemNum->setTextAlignment(Qt::AlignCenter);
        itemMiss->setTextAlignment(Qt::AlignCenter);
        itemTime->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Цвета призёров
        if (place == 1) {
            itemPlace->setForeground(gold);
            itemName->setForeground(gold);
        } else if (place == 2) {
            itemPlace->setForeground(silver);
            itemName->setForeground(silver);
        } else if (place == 3) {
            itemPlace->setForeground(bronze);
            itemName->setForeground(bronze);
        }

        if (place <= 3) {
            QFont bold = itemName->font();
            bold.setBold(true);
            itemPlace->setFont(bold);
            itemName->setFont(bold);
        }

        resultsTable->setItem(i, 0, itemPlace);
        resultsTable->setItem(i, 1, itemNum);
        resultsTable->setItem(i, 2, itemName);
        resultsTable->setItem(i, 3, itemMiss);
        resultsTable->setItem(i, 4, itemTime);
    }
}

// Когда кликаешь на спортсмена в таблице — показывает всю инфу о нём
void BiathlonApp::onAthleteSelected(int row, int col) {
    Q_UNUSED(col);
    if (row < 0 || row >= (int)participants.size())
        return;

    auto &p = participants[row];
    const Biathlete &info = p->info;

    QString html = QString(
        "<h3>%1 (%2)</h3>"
        "<table style='width:100%; color:#000000;'>"
        "<tr><td><b>Возраст:</b></td><td>%3 лет</td></tr>"
        "<tr><td><b>Рост:</b></td><td>%4 см</td></tr>"
        "<tr><td><b>В спорте:</b></td><td>%5 лет</td></tr>"
        "<tr><td><b>Сильные стороны:</b></td><td>%6</td></tr>"
        "<tr><td><b>Скорость:</b></td><td>%7 сек/км</td></tr>"
        "<tr><td><b>Точность:</b></td><td>%8%</td></tr>"
        "<tr><td><b>Выносливость:</b></td><td>%9/100</td></tr>"
        "</table><hr><b>Достижения:</b><br>%10"
    ).arg(info.name).arg(info.country)
     .arg(info.age).arg(info.height)
     .arg(info.experience).arg(info.strengths)
     .arg(info.runningSpeed, 0, 'f', 1)
     .arg(info.shootingAccuracy * 100, 0, 'f', 0)
     .arg(info.endurance).arg(info.achievements);

    athleteInfoLabel->setText(html);
    mainTabs->setCurrentIndex(1);
}
