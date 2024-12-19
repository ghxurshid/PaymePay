#include <WiFi.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "MenuManager.h"
#include "temp.h"

static int amount = 0;
static String token;
static int balance = 0;
const char* paycomUrl = "https://checkout.test.paycom.uz/api";
const char* apiKey = "5e730e8e0b852a417aa49ceb:ZPDODSiTYKuX0jyO7Kl2to4rQbNwG08jbghj";
 
const int pinDataBalance = 19;

WiFiClientSecure client;
 
// Реализация Event
Event::Event(EventType type) : m_type(type) {}

EventType Event::eventType() const {
    return m_type;
}

// Реализация BaseMenu
BaseMenu::BaseMenu() : m_next(nullptr), m_prev(nullptr) {}

BaseMenu::~BaseMenu() {}

void BaseMenu::setNavigations(BaseMenu* next, BaseMenu* prev, std::vector<BaseMenu*>& pages) {
    m_next = next;
    m_prev = prev;
    m_pages = pages;
}

BaseMenu* BaseMenu::next() {
  return m_next;
}

BaseMenu* BaseMenu::prev() {
  return m_prev;
}

void BaseMenu::show() {
  draw();
  onShow();
}

bool BaseMenu::shouldNext() {
  bool r = m_shouldNext;
  m_shouldNext = false;
  return r;
}

// Реализация WelcomeMenu
WelcomeMenu::WelcomeMenu() {}

void WelcomeMenu::onShow() {
    m_shouldNext = true;
}

void WelcomeMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void WelcomeMenu::onHide() {
    //not implemented
}

void WelcomeMenu::draw() {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Welcome!!!");
    delay(1000);    
}

// Реализация InitializationMenu
InitializationMenu::InitializationMenu() {}

void InitializationMenu::onShow() {
    m_shouldNext = true;
}

void InitializationMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void InitializationMenu::onHide() {
    
}

void InitializationMenu::draw() {
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print("Initialization");

    delay(500);    

    lcd.clear();

    lcd.setCursor(5,0);
    lcd.print("Wi-Fi");
     
    lcd.setCursor(1,1);
    lcd.print("Connecting...");
    delay(1000);
}

// Реализация MainMenu
MainMenu::MainMenu() {}

void MainMenu::onShow() {    
    
}

void MainMenu::handleEvent(MenuContainer* container, Event* evnt) {
  auto ev = evnt->eventType();
  switch (ev) {
    case BUTTON_UP_PRESSED: 
      m_next = m_pages[1];
      m_shouldNext = true;
      break;

    case BUTTON_OK_PRESSED: 
      m_next = m_pages[0];
      m_shouldNext = true;
      break;
  }  
}

void MainMenu::onHide() {
     
}

void MainMenu::draw() {
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("BOT");

  lcd.setCursor(2,1);
  lcd.print("Electro Prog");
}

// Реализация InputAmountMenu
InputAmountMenu::InputAmountMenu() {}

void InputAmountMenu::onShow() {
    m_amount = 5000;
}

void InputAmountMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) 
    {
      m_amount = std::min(100000, m_amount + 1000);
      draw();
    }
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) 
    {
      m_amount = std::max(1000, m_amount - 1000);
      draw();
    } 
    else if (evnt->eventType() == BUTTON_OK_PRESSED)
    {
      amount = m_amount;
      m_shouldNext = true;
    }
}

void InputAmountMenu::onHide() {
    //not implemented
}

void InputAmountMenu::draw() {
    lcd.clear();
    lcd.print("Amount: ");
    lcd.print(m_amount);
}

// Реализация PayMenu
PayMenu::PayMenu() {}

void printError(const String& message, const String& value = "") {
  lcd.clear();
  lcd.print(message);
  if (value != "") lcd.print(value);
  delay(1000);
}

String sendHttpRequest(const String& url, const String& authKey, const String& payload, int& httpCode) {  
  client.setInsecure();
 
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("X-Auth", authKey);
  http.addHeader("Content-Type", "application/json");

  Serial.println(url);
  Serial.println(authKey);
  Serial.println(payload);

  httpCode = http.POST(payload); // HTTP POST запрос
  String response = httpCode == 200 ? http.getString() : String("");    
  http.end();

  return response;
}

bool deserializeJsonResponse(const String& response, DynamicJsonDocument& doc) {
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    printError("JSON Error: ", error.c_str());
    return false;
  }
  return true;
}

void processPayment(const String& paycomUrl, const String& apiKey, const String& token, const String& receiptId, int& amount, int& balance) {
  String payload = String("{") +
                   "\"id\": 123," +
                   "\"method\": \"receipts.pay\"," +
                   "\"params\": {" +
                   "\"id\": \"" + receiptId + "\"," +
                   "\"token\": \"" + token + "\"" +
                   "}" +
                   "}";

  int httpResponseCode = 0;
  String response = sendHttpRequest(paycomUrl, apiKey, payload, httpResponseCode);

  Serial.println(response);

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(response.length() * 3);
    if (deserializeJsonResponse(response, doc)) {
      JsonObject receipt = doc["result"]["receipt"];
      if (receipt.containsKey("state")) {
        int state = receipt["state"];
        if (state == 4) {
          balance = amount;
          amount = 0;
          lcd.clear();
          lcd.print("Payment Success!");
          delay(1000);
        } else {
          printError("Unsucces: ", String(state));
        }
      } else {
        printError("Error: no State");
      }
    }
  } else {
    printError("Error P: ", String(httpResponseCode));
  }
}

void createReceipt(const String& paycomUrl, const String& apiKey, const String& token, int& amount, int& balance) {
  Serial.print(1);
  String payload = String("{") +
                   "\"id\": 3465434565," +
                   "\"method\": \"receipts.create\"," +
                   "\"params\": {" +
                   "\"amount\": " + String(amount * 100) + "," +
                   "\"account\": {\"order_id\": \"64543\"}" +
                   "}" +
                   "}";
  Serial.print(2);
  int httpResponseCode = 0;
  String response = sendHttpRequest(paycomUrl, apiKey, payload, httpResponseCode);
  Serial.print(3);
  Serial.println(response);
  return;

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(response.length() * 3);
    if (deserializeJsonResponse(response, doc)) {
      const char* receiptId = doc["result"]["receipt"]["_id"];
      if (receiptId != nullptr) {
        processPayment(paycomUrl, apiKey, token, receiptId, amount, balance);
      } else {
        printError("Error: no ID");
      }
    }
  } else {
    printError("Error C: ", String(httpResponseCode));
  }
}
const char* serverURL = "https://checkout.test.paycom.uz/api"; // URL API
void PayMenu::onShow() {
  // Запуск процесса
    WiFiClientSecure client1; // Инициализация WiFiClientSecure

    client1.setInsecure();
    //createReceipt(paycomUrl, apiKey, token, amount, balance);
    HTTPClient http;
    http.begin(client1, serverURL);  // Устанавливаем URL для запроса и используем WiFiClientSecure

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Auth", "5e730e8e0b852a417aa49ceb:ZPDODSiTYKuX0jyO7Kl2to4rQbNwG08jbghj");
    http.addHeader("Cache-Control", "no-cache");

    // JSON данные для отправки
    String jsonData = R"(
      {
        "id": 3465434565,
        "method": "receipts.create",
        "params": {
          "amount": 10000,
          "account": {
            "order_id": "64543"
          }
        }
      }
    )";

    int httpResponseCode = http.POST(jsonData);  // Выполнение POST-запроса

    // Проверяем код ответа
    if (httpResponseCode > 0) {
      String response = http.getString();  // Получаем ответ от сервера
      Serial.print("Ответ сервера: ");
      Serial.println(response);
    } else {
      Serial.print("Ошибка при выполнении POST-запроса: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Завершаем соединение      

  delay(3000);
  m_shouldNext = true;
}

void PayMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void PayMenu::onHide() {
    
}

void PayMenu::draw() {
    lcd.clear();
    lcd.print("Create receipt");
    delay(500);    
}

// Реализация SettingsMenu
SettingsMenu::SettingsMenu() {}

void SettingsMenu::onShow() {
    lcd.clear();
    lcd.print("Set onSHow");
    delay(1000);
}

void SettingsMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void SettingsMenu::onHide() {
    
}

void SettingsMenu::draw() {
  
}

// Реализация WifiSettingsMenu
WifiSettingsMenu::WifiSettingsMenu() {}

void WifiSettingsMenu::onShow() {
    lcd.clear();
    lcd.print("WiFi onSHow");
    delay(1000);
}

void WifiSettingsMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void WifiSettingsMenu::onHide() {
    
}

void WifiSettingsMenu::draw() {
  
}
 
// Реализация NotificationSettingsMenu
NotificationSettingsMenu::NotificationSettingsMenu() {}

void NotificationSettingsMenu::onShow() {
    lcd.clear();
    lcd.print("Notif Settings");    
}

void NotificationSettingsMenu::handleEvent(MenuContainer* container, Event* evnt) {
    if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
    else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void NotificationSettingsMenu::onHide() {
   
}

void NotificationSettingsMenu::draw() {
  
}

// Реализация MenuContainer
MenuContainer::MenuContainer() : m_currentMenu(nullptr) {
    // Создаем все меню
    auto welcomeMenu = new WelcomeMenu();
    auto initializationMenu = new InitializationMenu();
    auto notificationSettingsMenu = new NotificationSettingsMenu();

    auto mainMenu = new MainMenu();
    auto inputMenu = new InputAmountMenu();
    auto payMenu = new PayMenu();
    auto settingsMenu = new SettingsMenu();
    auto wifiSettingsMenu = new WifiSettingsMenu();

    // Устанавливаем связи между меню
    std::vector<BaseMenu*> welcomePages = {mainMenu};
    welcomeMenu->setNavigations(initializationMenu, nullptr, welcomePages);

    std::vector<BaseMenu*> initPages = {mainMenu};
    initializationMenu->setNavigations(mainMenu, welcomeMenu, initPages);

    std::vector<BaseMenu*> mainMenuPages = {inputMenu, settingsMenu};
    mainMenu->setNavigations(inputMenu, initializationMenu, mainMenuPages);

    std::vector<BaseMenu*> inputMenuPages = {};
    inputMenu->setNavigations(payMenu, mainMenu, inputMenuPages);

    std::vector<BaseMenu*> payMenuPages = {};
    payMenu->setNavigations(mainMenu, inputMenu, payMenuPages);

    std::vector<BaseMenu*> settingsMenuPages = {wifiSettingsMenu, notificationSettingsMenu};
    settingsMenu->setNavigations(mainMenu, nullptr, settingsMenuPages);

    std::vector<BaseMenu*> wifiSettingsMenuPages = {};
    wifiSettingsMenu->setNavigations(nullptr, settingsMenu, wifiSettingsMenuPages);

    std::vector<BaseMenu*> notifSettingsMenuPages = {};
    notificationSettingsMenu->setNavigations(settingsMenu, nullptr, notifSettingsMenuPages);

    // Добавляем меню в список
    m_menuList.push_back(welcomeMenu);
    m_menuList.push_back(initializationMenu);
    m_menuList.push_back(mainMenu);
    m_menuList.push_back(inputMenu);
    m_menuList.push_back(payMenu);
    m_menuList.push_back(settingsMenu);
    m_menuList.push_back(wifiSettingsMenu);
    m_menuList.push_back(notificationSettingsMenu);

    // Устанавливаем начальное меню
    m_currentMenu = welcomeMenu;    
}


void MenuContainer::handleEvent(Event* evnt) {
  if (m_currentMenu == nullptr)
    m_currentMenu = m_menuList.front();

  if (evnt->eventType() == EventType::START) {       
      m_currentMenu->show();          
  } else if(evnt->eventType() == EventType::TICK) {
    digitalWrite(pinDataBalance, balance > 0 ? HIGH : LOW);
    balance = std::max(0, balance - 1000);
  } else {    
      m_currentMenu->handleEvent(this, evnt);          
  }
  while (m_currentMenu->shouldNext()) next();  
}

void MenuContainer::next() { 
  if (m_currentMenu == nullptr) return;
  if (m_currentMenu->next() == nullptr) return;

  m_currentMenu->onHide();
  m_currentMenu = m_currentMenu->next();
  m_currentMenu->show();  
}

void MenuContainer::prev() {
  if (m_currentMenu == nullptr) return;
  if (m_currentMenu->prev() == nullptr) return;
 
  m_currentMenu->onHide();
  m_currentMenu = m_currentMenu->prev();
  m_currentMenu->show();
}

