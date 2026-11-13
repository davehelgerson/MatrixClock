# 1 "/tmp/tmpo9bbqz43"
#include <Arduino.h>
# 1 "/home/dadmin/Documents/Workspace/personal/MatrixClock/Clock/Clock.ino"
# 20 "/home/dadmin/Documents/Workspace/personal/MatrixClock/Clock/Clock.ino"
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "Clockface.h"
#include "CWDateTime.h"
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include "touch.h"
#include "pin_config.h"
#include "RTClib.h"


static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 3 , true );

volatile int counter = 0;
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;
int State;
int old_State;
int move_flag = 0;

volatile int brightness = 30;


int page_index = 0;
volatile int gif_num = 0;
volatile int fresh_flag = 0;
volatile bool screen_dimmed = false;

MatrixPanel_I2S_DMA *dma_display = nullptr;

CWDateTime cwDateTime;

Clockface1 *clockface1;
Clockface2 *clockface2;
Clockface3 *clockface3;
Clockface4 *clockface4;
Clockface5 *clockface5;
RTC_PCF8563 rtc;

#define panelResX 64
#define panelResY 64
#define panel_chain 1

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

volatile unsigned long screen_lastMillis = 0;
volatile int screen_sleep = 0;
volatile int screen_weak_up = 0;
void displaySetup();
void setup();
void loop();
void Task_TFT(void *pvParameters);
void Task_Main(void *pvParameters);
void Task_Time(void *pvParameters);
void Task_screen_sleep(void *pvParameters);
void time_bg(int bg);
void update_time(int time);
void pin_init();
void IRAM_ATTR encoder_irq();
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void encoder_func();
void obj_update();
#line 79 "/home/dadmin/Documents/Workspace/personal/MatrixClock/Clock/Clock.ino"
void displaySetup()
{
  HUB75_I2S_CFG mxconfig(
    panelResX,
    panelResY,
    panel_chain
    );


  dma_display = new MatrixPanel_I2S_DMA(mxconfig);


  dma_display->begin(R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN);
  dma_display->setBrightness(map(brightness, 0, 100, 0, 255));
  dma_display->clearScreen();
}

void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.println("Starting up...");

  pin_init();
  gfx->begin();

  delay(200);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);


  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);


  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();

  displaySetup();


  clockface1 = new Clockface1(dma_display);
  clockface2 = new Clockface2(dma_display);
  clockface3 = new Clockface3(dma_display);
  clockface4 = new Clockface4(dma_display);
  clockface5 = new Clockface5(dma_display);

  cwDateTime.begin();

  clockface3->setup(&cwDateTime);
  Serial.println("Setup done");

  xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 10240, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Task_Main, "Task_Main", 4096, NULL, 1, NULL, 0);
  delay(2500);
  xTaskCreatePinnedToCore(Task_Time, "Task_Time", 102400, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(Task_screen_sleep, "Task_screen_sleep", 10240, NULL, 1, NULL, 0);
}

void loop()
{
}

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        obj_update();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Task_Main(void *pvParameters)
{
    while (1)
    {
        encoder_func();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Task_Time(void *pvParameters)
{
  int active_gif = 0;
  while(1)
  {
      int target_gif = gif_num;
      if (target_gif != active_gif)
      {
          vTaskDelay(pdMS_TO_TICKS(150));
          target_gif = gif_num;
          active_gif = target_gif;
          time_bg(active_gif);
      }
      update_time(active_gif);
      vTaskDelay(1);
  }
}

void Task_screen_sleep(void *pvParameters)
{
    const unsigned long dimTimeout = 300000UL;
    const unsigned long sleepTimeout = 3600000UL;



    while (1)
    {
        unsigned long idleTime = millis() - screen_lastMillis;

        if (!screen_sleep && idleTime >= sleepTimeout)
        {
            screen_sleep = 1;
            screen_dimmed = false;
            gfx->displayOff();
            dma_display->setBrightness(0);
        }
        else if (!screen_sleep && !screen_dimmed && idleTime >= dimTimeout && brightness > 5)
        {
            screen_dimmed = true;
            dma_display->setBrightness(map(5, 0, 100, 0, 255));
        }

        if (screen_weak_up == 1)
        {
            gfx->displayOn();
            dma_display->setBrightness(map(brightness, 0, 100, 0, 255));
            screen_lastMillis = millis();
            screen_weak_up = 0;
            screen_sleep = 0;
            screen_dimmed = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void time_bg(int bg)
{
  switch(bg)
  {
    case 0:
    clockface3->setup(&cwDateTime);
    clockface3->update();
    fresh_flag=0;break;
    case 1:
    clockface2->setup(&cwDateTime);
    clockface2->update();
    fresh_flag=0;break;
    case 2:
    clockface1->setup(&cwDateTime);
    clockface1->update();
    fresh_flag=0;break;
    case 3:
    clockface4->setup(&cwDateTime);
    clockface4->update();
    fresh_flag=0;break;
    case 4:
    clockface5->setup(&cwDateTime);
    clockface5->update();
    fresh_flag=0;break;
  }
}

void update_time(int time)
{
  if(time==0)
  {
    clockface3->update();
  }
  else if(time==1)
  {
    clockface2->update();
  }
  else if(time==2)
  {
    clockface1->update();
  }
  else if(time==3)
  {
    clockface4->update();
  }
  else if(time==4)
  {
    clockface5->update();
  }
}

void pin_init()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    old_State = digitalRead(ENCODER_CLK);

    attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);
}

void IRAM_ATTR encoder_irq()
{
    State = digitalRead(ENCODER_CLK);
    if (State != old_State)
    {
        portENTER_CRITICAL_ISR(&encoderMux);
        if (digitalRead(ENCODER_DT) == State)
        {
            counter++;
        }
        else
        {
            counter--;
        }
        portEXIT_CRITICAL_ISR(&encoderMux);

        if(screen_sleep==1 || screen_dimmed)
        {
            screen_weak_up= 1;
        }
    }
    old_State = State;
}


void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}


void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int touchX = 0, touchY = 0;

    if (read_touch(&touchX, &touchY) == 1)
    {
        data->state = LV_INDEV_STATE_PR;

        data->point.x = (uint16_t)(240 - touchY);
        data->point.y = (uint16_t)touchX;

        if(screen_sleep==1 || screen_dimmed)
        {
            screen_weak_up= 1;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void encoder_func()
{
    portENTER_CRITICAL(&encoderMux);
    int delta = counter;
    counter = 0;
    portEXIT_CRITICAL(&encoderMux);

    if (delta == 0)
    {
        return;
    }

    screen_lastMillis = millis();

    if (page_index == 0)
    {
        int next_gif = ((gif_num + delta) % 5 + 5) % 5;
        gif_num = next_gif;

        Serial.print("gif_num=");
        Serial.println(next_gif);
        fresh_flag = 1;
    }
    else if (page_index == 1)
    {
        int next_brightness = brightness + delta;
        if (next_brightness < 0)
            next_brightness = 0;
        if (next_brightness > 100)
            next_brightness = 100;
        brightness = next_brightness;

        dma_display->setBrightness(map(next_brightness, 0, 100, 0, 255));
    }
}

void obj_update()
{
    static int last_ui_page = -1;
    static int last_ui_gif = -1;
    static int last_ui_brightness = -1;

    int cur_page = page_index;
    int cur_gif = gif_num;
    int cur_brightness = brightness;

    if (cur_page == 0 && (cur_page != last_ui_page || cur_gif != last_ui_gif))
    {
        lv_roller_set_selected(ui_Roller1, cur_gif, LV_ANIM_ON);
        last_ui_gif = cur_gif;
        last_ui_page = cur_page;
    }
    else if (cur_page == 1 && (cur_page != last_ui_page || cur_brightness != last_ui_brightness))
    {
        char temp[16] = "";
        lv_arc_set_value(ui_Arc1, cur_brightness);

        snprintf(temp, sizeof(temp), "%d%%", cur_brightness);
        lv_label_set_text(ui_Label1, temp);
        last_ui_brightness = cur_brightness;
        last_ui_page = cur_page;
    }
}