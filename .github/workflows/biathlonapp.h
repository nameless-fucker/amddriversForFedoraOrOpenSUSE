#ifndef BIATHLONAPP_H
#define BIATHLONAPP_H

#include <QWidget>         // базовый класс окон
#include <QComboBox>       // выпадающий список дисциплин
#include <QTextEdit>       // лог событий гонки
#include <QPushButton>     // кнопки управления
#include <QTableWidget>    // таблица результатов
#include <QTabWidget>      // вкладки "Гонка" и "Профиль"
#include <QLabel>          // подписи и информация о спортсмене
#include <vector>          // динамические массивы (участники, спортсмены)
#include <random>          // генератор случайных чисел для симуляции
#include <memory>          // умные указатели shared_ptr

// Тут храним инфу о спортсмене
struct Biathlete {
    QString name;              // имя и фамилия
    QString country;           // страна
    int age;                   // возраст
    int height;                // рост в см
    int experience;            // лет в спорте
    QString achievements;      // достижения
    QString strengths;         // сильные стороны
    double runningSpeed;       // скорость бега (сек/км)
    double shootingAccuracy;   // точность стрельбы (0.0–1.0)
    int shootingTime;          // время на стрельбу (сек)
    int endurance;             // выносливость (0–100)
};

// Класс для участника гонки — добавляет номер, время и промахи к данным спортсмена
class RaceParticipant {
public:
    Biathlete info;
    int startNumber;       // стартовый номер
    double totalTime;      // итоговое время
    int missedShots;       // сколько промахов
    bool finished;         // завершил ли гонку

    // Задаётся спортсмен и номер, всё остальное = 0
    RaceParticipant(const Biathlete &b, int num)
        : info(b), startNumber(num), totalTime(0.0), missedShots(0), finished(false) {}
};

// Виды гонок — для каждого своя дистанция и сколько раз стрелять
enum class RaceType {
    Sprint,      // спринт 10 км
    Pursuit,     // преследование 12.5 км
    Individual,  // индивидуальная 20 км
    MassStart    // масс-старт 15 км
};

// Главный класс программы — тут всё управление и интерфейс
class BiathlonApp : public QWidget {
    Q_OBJECT

public:
    // Конструктор — инициализирует окно и элементы интерфейса
    BiathlonApp(QWidget *parent = nullptr);

private slots:
    // Жеребьёвка: перемешивает спортсменов, назначает стартовые номера
    void onGenerateStartList();
    // Старт: запускает симуляцию прохождения дистанции и стрельбы
    void onStartRace();
    // Результаты: сортирует и выводит итоговую таблицу с местами
    void onShowResults();
    // Клик по строке таблицы — показывает полный профиль спортсмена
    void onAthleteSelected(int row, int col);

private:
    // Симуляция одного огневого рубежа (5 выстрелов)
    void simShooting(RaceParticipant &p, int lapNumber, int totalLaps);
    // Симуляция бега на одном круге дистанции
    double simRunning(const RaceParticipant &p, double distanceKm, int lapNumber, int totalLaps);
    // Сортировка участников по возрастанию итогового времени
    void calculateResults();
    // Настройка внешнего вида таблицы результатов (белая тема)
    void setupTableStyle();
    // Заполнение массива спортсменов начальными данными
    void initAthletes();

    // Интерфейс (кнопки, таблицы, вкладки)
    QComboBox *raceTypeCombo;       // выбор дисциплины
    QTextEdit *outputLog;           // лог событий
    QPushButton *btnGenerate;       // жеребьёвка
    QPushButton *btnStart;          // старт
    QPushButton *btnResults;        // результаты
    QTableWidget *resultsTable;     // таблица результатов
    QLabel *athleteInfoLabel;       // профиль участника
    QTabWidget *mainTabs;           // вкладки

    // Данные программы
    std::vector<std::shared_ptr<RaceParticipant>> participants;  // список участников
    RaceType currentRace;            // текущая дисциплина
    std::vector<Biathlete> teamPool; // все спортсмены
    std::mt19937 rng;               // генератор случайных чисел
};

#endif
