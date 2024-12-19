#ifndef MENU_MANAGER
#define MENU_MANAGER

#include <vector>
#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;

// Forward declaration
class MenuContainer;

// Типы событий
enum EventType {
    START,
    BUTTON_OK_PRESSED,
    BUTTON_UP_PRESSED,
    BUTTON_DOWN_PRESSED,
    UART_RECEIVED,
    TICK,
    UPDATE
};

// Класс событий
class Event {
  public:
    Event(EventType type);
    EventType eventType() const;

  private:
    EventType m_type;
};

// Базовый класс меню
class BaseMenu {
  public:
    BaseMenu();
    virtual void onShow() = 0;
    virtual void handleEvent(MenuContainer* container, Event* evnt) = 0;
    virtual void onHide() = 0;

    virtual void draw() = 0;

    virtual ~BaseMenu();

    void setNavigations(BaseMenu* next, BaseMenu* prev, std::vector<BaseMenu*>& pages);
    BaseMenu* next();
    BaseMenu* prev();

    void show();
    bool shouldNext();

  protected:
    BaseMenu* m_next;     // Ссылка на следующее меню
    BaseMenu* m_prev;     // Ссылка на предыдущее меню             
    std::vector<BaseMenu*> m_pages; // Вектор подменю
    bool m_shouldNext = false; //Флаг для информирования следуюшей
};

// Главное меню
class MainMenu : public BaseMenu {
  public:
    MainMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню ввода суммы
class InputAmountMenu : public BaseMenu {
  public:
    InputAmountMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;

  protected:
    int m_amount = 5000;
};

// Меню оплаты
class PayMenu : public BaseMenu {
  public:
    PayMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню настроек
class SettingsMenu : public BaseMenu {
  public:
    SettingsMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню WiFi-настроек
class WifiSettingsMenu : public BaseMenu {
  public:
    WifiSettingsMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Приветственное меню
class WelcomeMenu : public BaseMenu {
  public:
    WelcomeMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню инициализации
class InitializationMenu : public BaseMenu {
  public:
    InitializationMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню настроек уведомлений
class NotificationSettingsMenu : public BaseMenu {
  public:
    NotificationSettingsMenu();

    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Контейнер для управления меню
class MenuContainer {
  public:
    MenuContainer();

    void handleEvent(Event* evnt); // Обработка события
    void next();
    void prev();

  private:
    BaseMenu* m_currentMenu;       // Текущее активное меню
    std::vector<BaseMenu*> m_menuList; // Список всех меню
};

#endif

