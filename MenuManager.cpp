#include <WiFi.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "MenuManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_heap_caps.h>

static int amount = 0;
static String token;
static int balance = 0;
const char* paycomUrl = "https://checkout.test.paycom.uz/api";
const char* apiKey = "5e730e8e0b852a417aa49ceb:ZPDODSiTYKuX0jyO7Kl2to4rQbNwG08jbghj";

const char* ssid = "mr_home";       // Название Wi-Fi
const char* password = "46981097";  // Пароль Wi-Fi

const int pinDataBalance = 19;

int calcIndent(const String& text) {
  int cnt = text.length();
  return std::max(0, (20 - cnt) / 2);
}

// Реализация Event
Event::Event(EventType type) : m_type(type) {}

EventType Event::eventType() const {
  return m_type;
}

// Реализация BaseMenu
BaseMenu::BaseMenu() : m_next(nullptr), m_prev(nullptr) {}

BaseMenu::~BaseMenu() {}

void BaseMenu::draw() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(m_title);
  lcd.setCursor(calcIndent(m_message1),1); lcd.print(m_message1);
  lcd.setCursor(calcIndent(m_message2),2); lcd.print(m_message2);
  lcd.setCursor(calcIndent(m_message3),3); lcd.print(m_message3);
}

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
  onShowing();
  draw();
  onShow();
}

bool BaseMenu::shouldNext() {
  bool r = m_shouldNext;
  m_shouldNext = false;
  return r;
}

// Реализация WelcomeMenu
WelcomeMenu::WelcomeMenu() {   
  m_message1 = String("Welcome!");
}

void WelcomeMenu::onShowing() {
  
}

void WelcomeMenu::onShow() {
  delay(1000);
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
  BaseMenu::draw();
}

// Реализация InitializationMenu
InitializationMenu::InitializationMenu() {
  m_message1 = String("Initialization");
}

void InitializationMenu::onShowing() {
}

void InitializationMenu::onShow() {
  delay(1000);
  m_message1 = String("WiFi");
  m_message2 = String("Connecting");

  draw();

  WiFi.begin(ssid, password);

  int tryCnt = 0;
  // Ожидание подключения
  while (WiFi.status() != WL_CONNECTED) {    
    m_message3 += ".";
    draw();
    delay(1000);
    tryCnt ++;
    if (tryCnt > 19) break;
  }

  delay(1000);
  m_shouldNext = true;
}

void InitializationMenu::handleEvent(MenuContainer* container, Event* evnt) {
  //not implemented
}

void InitializationMenu::onHide() {
  //not implemented
}

void InitializationMenu::draw() {
  BaseMenu::draw();
}

// Реализация MainMenu
MainMenu::MainMenu() {
  m_title = String("_____Main__Menu_____");
  
}

void MainMenu::onShowing() {
  m_message2 = String(balance) + String(" sum");
}

void MainMenu::onShow() {
  //not implemented
}

void MainMenu::handleEvent(MenuContainer* container, Event* evnt) {
  auto ev = evnt->eventType();
  switch (ev) {
    case BUTTON_UP_PRESSED:
      m_next = m_pages[1];
      m_shouldNext = true;
      break;

    case UART_ON_RECEIVED:
      m_next = m_pages[0];
      m_shouldNext = true;
      break;
  }
}

void MainMenu::onHide() {
  m_message1.clear();
  m_message2.clear();
  m_message3.clear();
}

void MainMenu::draw() {
  BaseMenu::draw();
}

// Реализация InputAmountMenu
InputAmountMenu::InputAmountMenu() {  
  m_title = String("____Enter_amount____");  
}

void InputAmountMenu::onShowing() {
  m_amount = 5000;
  m_message2 = String(m_amount) + String("som");
}

void InputAmountMenu::onShow() {
  String input = ""; // Переменная для сохранения данных
  while (Serial.available()) {
    char received = Serial.read(); // Считываем один символ
    input += received;             // Добавляем символ в строку
    delay(2);                      // Небольшая задержка для предотвращения потери данных
  }
  token = input;
  m_shouldNext = true;
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
    m_shouldNext = true;
  }
}

void InputAmountMenu::onHide() {
  amount = m_amount;
  m_message1.clear();
  m_message2.clear();
  m_message3.clear();
}

void InputAmountMenu::draw() {
  BaseMenu::draw();
}

// Реализация PayMenu
PayMenu::PayMenu() {
  m_title = String("____Proccess_Pay____");
  m_message2 = String("Create check");
}

void checkStack() {
  UBaseType_t stackSize = configMINIMAL_STACK_SIZE; // Размер стека задачи (может быть другим, если вы создавали задачу с другим размером)
  UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL); // Получаем оставшийся стек

  float stackUsagePercent = 100.0f * (1.0f - ((float)highWaterMark / (float)stackSize));
  Serial.print("Stack Usage: ");
  Serial.print(stackUsagePercent);
  Serial.println("%");
  delay(1000);
}

void checkHeap() {
  size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT); // Общий размер кучи
  size_t freeHeap = esp_get_free_heap_size(); // Текущая свободная куча

  float heapUsagePercent = 100.0f * (1.0f - ((float)freeHeap / (float)totalHeap));
  Serial.print("Heap Usage: ");
  Serial.print(heapUsagePercent);
  Serial.println("%");
}

void printProgramMemoryUsage() {
  size_t sketchSize = ESP.getSketchSize();       // Размер текущего скетча
  size_t totalFlash = ESP.getFlashChipSize();    // Общий объем флэш-памяти
  size_t freeFlash = ESP.getFreeSketchSpace();   // Доступное пространство для скетча

  Serial.println("=== Program Memory Usage ===");
  Serial.printf("Sketch Size: %u bytes\n", sketchSize);
  Serial.printf("Free Space: %u bytes\n", freeFlash);
  Serial.printf("Total Flash Size: %u bytes\n", totalFlash);
}

void printSRAMUsage() {
  size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);  // Общая куча
  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);   // Свободная куча
  size_t usedHeap = totalHeap - freeHeap;                      // Занятая куча

  Serial.println("=== Dynamic Memory Usage (SRAM) ===");
  Serial.printf("Total Heap: %u bytes\n", totalHeap);
  Serial.printf("Used Heap: %u bytes\n", usedHeap);
  Serial.printf("Free Heap: %u bytes\n", freeHeap);
}

String sendHttpRequest(const String& url, const String& authKey, const String& payload, int& httpCode) {
  WiFiClientSecure client;
  client.setInsecure();
  Serial.println("- - - - - - - - -");
  Serial.println(payload);

  HTTPClient http;
  http.begin(client, url);
  http.addHeader("X-Auth", authKey);
  http.addHeader("Content-Type", "application/json");

  httpCode = http.POST(payload); // HTTP POST запрос
  String response = httpCode == 200 ? http.getString() : String("");
  http.end();

  Serial.println(response);
  Serial.println("- - - - - - - - -");

  return response;
}

bool deserializeJsonResponse(const String& response, DynamicJsonDocument& doc) {
  DeserializationError error = deserializeJson(doc, response);
  if (error) {    
    return false;
  }
  return true;
}

bool PayMenu::processPayment(const String& paycomUrl, const String& apiKey, const String& token, const String& receiptId) {
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

  bool success = false;

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(response.length() * 3);
    if (deserializeJsonResponse(response, doc)) {
      JsonObject receipt = doc["result"]["receipt"];
      if (receipt.containsKey("state")) {
        int state = receipt["state"];
        if (state == 4) {           
          m_message2 = String("Payment Success!"); 
          success = true;          
        } else {           
          m_message2 = String("Unsucces:") + String(state);
        }
      } else {
        m_message2 = String("Error: no State");        
      }
    } else {
      m_message2 = String("JSON Error:");      
    }
  } else {
    m_message2 = String("Pay HTTP Error:") + String(httpResponseCode);
  }
 
  return success;
}

String PayMenu::createReceipt(const String& paycomUrl, const String& apiKey, int amount) {
  m_message2 = String("Create Receipt");
  draw();

  String payload = String("{") +
                   "\"id\": 3465434565," +
                   "\"method\": \"receipts.create\"," +
                   "\"params\": {" +
                   "\"amount\": " + String(amount * 100) + "," +
                   "\"account\": {\"order_id\": \"64543\"}" +
                   "}" +
                   "}";

  int httpResponseCode = 0;
  String response = sendHttpRequest(paycomUrl, apiKey, payload, httpResponseCode);

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(response.length() * 3);
    if (deserializeJsonResponse(response, doc)) {
      const char* receiptId = doc["result"]["receipt"]["_id"];
      if (receiptId != nullptr) {
        return String(receiptId);
      } else {
        m_message2 = String("Error: no receiptId");
      }
    }
  } else {
    m_message2 = String("Create HTTP Err:") + String(httpResponseCode);
  }

  return String("");
}
 
void PayMenu::onShowing() {
  m_message2 = String("Create check");
}
 
void PayMenu::onShow() {   
  // Запуск процесса
  String receiptId = createReceipt(paycomUrl, apiKey, amount);
  if (receiptId.length() > 0) {
    m_message2 = String("Pay proccess");
    draw();
    bool result = processPayment(paycomUrl, apiKey, token, receiptId);
    if (result) {
      balance = amount;       
    }
  }

  draw();
  delay(1000);
  m_shouldNext = true;
}

void PayMenu::handleEvent(MenuContainer* container, Event* evnt) {
  if (evnt->eventType() == BUTTON_UP_PRESSED) container->next();
  else if (evnt->eventType() == BUTTON_DOWN_PRESSED) container->prev();
}

void PayMenu::onHide() {
  amount = 0;
  m_message1.clear();
  m_message2.clear();
  m_message3.clear();
}

void PayMenu::draw() {
  BaseMenu::draw();
}

// Реализация SettingsMenu
SettingsMenu::SettingsMenu() {}

void SettingsMenu::onShowing() {
  //not implemented
}

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
  BaseMenu::draw();
}

// Реализация WifiSettingsMenu
WifiSettingsMenu::WifiSettingsMenu() {}

void WifiSettingsMenu::onShowing() {

}
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
  BaseMenu::draw();
}

// Реализация NotificationSettingsMenu
NotificationSettingsMenu::NotificationSettingsMenu() {}

void NotificationSettingsMenu::onShowing() {
  //not implemented
}

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
  BaseMenu::draw();
}

// Реализация MenuContainer
MenuContainer::MenuContainer() : m_currentMenu(nullptr) {
  // Создаем все меню
  auto welcomeMenu = new WelcomeMenu();
  auto initializationMenu = new InitializationMenu();

  auto mainMenu = new MainMenu();

  auto inputMenu = new InputAmountMenu();
  auto payMenu = new PayMenu();

  auto settingsMenu = new SettingsMenu();
  auto wifiSettingsMenu = new WifiSettingsMenu();
  auto notificationSettingsMenu = new NotificationSettingsMenu();

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
  } else if (evnt->eventType() == EventType::TICK) {
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
