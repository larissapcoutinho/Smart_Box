/*---------------------------------------------------------
  Programa : Smart Box
  Autor    : Giuliana Britto, João Eduardo, Larissa Coutinho.
  ---------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <Wire.h>              // Biblioteca para comunicação I2C
#include <LiquidCrystal_I2C.h> // Biblioteca para controle do LCD
#include <Keypad.h>            // Biblioteca para leitura do teclado independente
#include <WiFi.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define PIN_LED 12
#define PIN_BUZZER 13
#define PIN_BUTTON 14
#define PIN_SCL 22
#define PIN_SDA 21

// Define as linhas e colunas do teclado matricial
const byte LINHAS = 4;
const byte COLUNAS = 3;
struct tm timeinf;
bool flag = true;
bool flag2 = false;
bool flag3 = false;
int maxAlarms = 0;

// ponteiros para vetores com memorias dinamicas
int* alarmHours;
int* alarmMin;
int* alarmControl;

char keys[LINHAS][COLUNAS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte pinos_linhas[LINHAS] = {12, 14, 27, 26};
byte pinos_colunas[COLUNAS] = {25, 33, 32};

Keypad keypad(makeKeymap(keys), pinos_linhas, pinos_colunas, LINHAS, COLUNAS);
LiquidCrystal_I2C lcd(0x27, PIN_SCL, PIN_SDA);

TaskHandle_t Task1;
WiFiUDP ntpUDP;

void task_menu(void *pvParameters);
void task_clock(void *pvParameters);
void defineAlarms(void);
void defineAlarmHours(void);
void defineAlarmMin(void);
void exit_menu(void);
void alarmRTC(void);
//void setClockRTC(void);

void setup(void)
{
    Serial.begin(115200);
    lcd.init();
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Smart Box");

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);

    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);

    WiFi.begin("Wokwi-GUEST", "", 6);
    Serial.print("Conectando no WiFi..");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nConectado!\n");

    xTaskCreatePinnedToCore(task_menu, "Task 1", 2000, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(task_clock, "Task 2", 4800, NULL, 1, NULL, 1);
}

void loop(void) { vTaskDelete(NULL); }

void task_menu(void *pvParameters)
{
    while (1)
    {
        char key = keypad.getKey();
        if (key != NO_KEY) // Quando a tecla for pressionada
        {
            lcd.setCursor(15, 1);
            lcd.print(key);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            switch (key)
            {
            case '*':
                lcd.setCursor(0, 0);
                lcd.print("Bem-vindo Menu");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                lcd.clear();
                defineAlarms();
                break;
            default:
                lcd.setCursor(0, 0);
                lcd.print("Opcao Invalida!");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                lcd.clear();
            }
        }
        lcd.setCursor(3, 0);
        lcd.print("Smart Box");
    //    lcd.setCursor(0, 1);
     //   lcd.print("00:00:00");
        vTaskDelay(1);
    }
}
void task_clock(void *pvParameters)
{
    while (1)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            configTime(-10800, 0, "pool.ntp.org");
            getLocalTime(&timeinf);
            Serial.println(&timeinf, "%d/%m/%Y, %a, %H:%M:%S");

            // setClockRTC();
        }
        alarmControl = (int *)malloc(maxAlarms * sizeof(int));

        for (int i = 0; i < maxAlarms; i++)
            alarmControl[i] = 1;

        // Percorre todos os alarmes
        for (int i = 0; i < maxAlarms; i++)
        {
            // Verifica se é hora de acionar o alarme
            if (timeinf.tm_hour == alarmHours[i] && timeinf.tm_min == alarmMin[i] && alarmControl[i])
            {
                alarmRTC();
                alarmControl[maxAlarms] = 0;
            }
        }

        if (timeinf.tm_hour == 23 && timeinf.tm_min == 59)
        {
            for (int i = 0; i < maxAlarms; i++)
            {
                alarmControl[maxAlarms] = 1;
            }
        }
        vTaskDelay(1);
    }
}
void defineAlarms(void)
{
    lcd.setCursor(0, 0);
    lcd.print("Insira Numero");
    lcd.setCursor(0, 1);
    lcd.print("de Alarmes:");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while (flag)
    {
        char key = keypad.getKey();
        if (key != NO_KEY)
        {
            switch (key)
            {
            case '#': // confirmar o numero de alarmes
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(maxAlarms);
                lcd.setCursor(3, 0);
                lcd.print("Alarmes");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                lcd.clear();
                flag2 = true;
                break;
            case '*':
                exit_menu();
                flag = false;
                break;
            default:
                maxAlarms = key - '0';
                lcd.setCursor(12, 1);
                lcd.print(maxAlarms);
                break;
            }
        }
        (flag2) ? defineAlarmHours() : void();
    }
    return;
}

void defineAlarmHours(void)
{
    int pos = 0;

    alarmHours = (int *)malloc(maxAlarms * sizeof(int));

    for (int i = 0; i < maxAlarms; i++)
        alarmHours[i] = 0;

    lcd.setCursor(0, 0);
    lcd.print("Inserir Horas");
    lcd.setCursor(0, 1);
    lcd.print("Alarme");
    lcd.setCursor(8, 1);
    lcd.print(":");

    while (flag2)
    {
        lcd.setCursor(7, 1);
        lcd.print(pos < maxAlarms ? pos + 1 : maxAlarms);

        char key = keypad.getKey();

        if (key != NO_KEY)
        {
            switch (key)
            {
            case '*':
                exit_menu();
                flag2 = false;
                flag = false;
                break;
            case '#':
                if (pos < maxAlarms)
                {
                    pos++;
                    lcd.setCursor(14, 1);
                    lcd.print(0);
                    lcd.setCursor(15, 1);
                    lcd.print(0);
                }
                else if (pos >= maxAlarms)
                {
                    flag3 = true;
                    pos = 0;
                }
                break;
            default:
                if (pos < maxAlarms)
                {
                    alarmHours[pos] = (alarmHours[pos] % 10) * 10 + (key - '0');
                    lcd.setCursor(14, 1);
                    lcd.print(alarmHours[pos]);
                }
                break;
            }
            for (int i = 0; i < maxAlarms; i++)
                Serial.println(alarmHours[i]);
        }
        (flag3) ? defineAlarmMin() : void();
    }
    free(alarmHours);
    return;
}

void defineAlarmMin(void)
{
    int pos = 0;

    alarmMin = (int *)malloc(maxAlarms * sizeof(int));

    for (int i = 0; i < maxAlarms; i++)
        alarmMin[i] = 0;

    // Exibir mensagem no display
    lcd.setCursor(0, 0);
    lcd.print("Inserir Minutos");
    lcd.setCursor(0, 1);
    lcd.print("Alarme");
    lcd.setCursor(8, 1);
    lcd.print(":");

    while (flag3)
    {
        lcd.setCursor(7, 1);
        lcd.print(pos < maxAlarms ? pos + 1 : maxAlarms);
        char key = keypad.getKey();

        if (key != NO_KEY)
        {
            switch (key)
            {
            case '*':
                exit_menu();
                flag3 = false;
                flag2 = false;
                flag = false;
                break;
            case '#':
                if (pos < maxAlarms)
                {
                    pos++;
                }
                else if (pos >= maxAlarms)
                {
                    pos = 0;
                    exit_menu();
                    flag3 = false;
                    flag2 = false;
                    flag = false;
                }

                break;
            default:
                if (pos < maxAlarms)
                {
                    alarmMin[pos] = (alarmMin[pos] % 10) * 10 + (key - '0');
                    lcd.setCursor(14, 1);
                    lcd.print(alarmMin[pos]);
                }
                break;
            }
            for (int i = 0; i < maxAlarms; i++)
                Serial.println(alarmMin[i]);
        }
    }
    free(alarmMin);
    return;
}

void exit_menu(void)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Saindo do Menu");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lcd.clear();
}


void alarmRTC(void)
{
    Serial.print("Alarme2 ");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}