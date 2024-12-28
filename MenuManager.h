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
    UART_ON_RECEIVED,
    TICK_UP,
    TICK_DOWN,
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
    virtual void onShowing() = 0;
    virtual void onShow() = 0;
    virtual void handleEvent(MenuContainer* container, Event* evnt) = 0;
    virtual void onHide() = 0;

    virtual void draw();

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
    String m_title = String("");
    String m_message1 = String("");
    String m_message2 = String("");
    String m_message3 = String("");
};

// Приветственное меню
class WelcomeMenu : public BaseMenu {
  public:
    WelcomeMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню инициализации
class InitializationMenu : public BaseMenu {
  public:
    InitializationMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;  
};

// Главное меню
class MainMenu : public BaseMenu {
  public:
    MainMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню ввода суммы
class InputAmountMenu : public BaseMenu {
  public:
    InputAmountMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;

  protected:
    int m_amount = 10;
};

// Меню оплаты
class PayMenu : public BaseMenu {
  public:
    PayMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;

  private:
    String createReceipt(const String& paycomUrl, const String& apiKey, int amount);
    bool processPayment(const String& paycomUrl, const String& apiKey, const String& token, const String& receiptId);
};

// Меню настроек
class SettingsMenu : public BaseMenu {
  public:
    SettingsMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню WiFi-настроек
class WifiSettingsMenu : public BaseMenu {
  public:
    WifiSettingsMenu();

    void onShowing() override;
    void onShow() override;
    void handleEvent(MenuContainer* container, Event* evnt) override;
    void onHide() override;
    void draw() override;
};

// Меню настроек уведомлений
class NotificationSettingsMenu : public BaseMenu {
  public:
    NotificationSettingsMenu();

    void onShowing() override;
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
