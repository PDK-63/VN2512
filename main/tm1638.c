#include "tm1638.h"
#include "esp_rom_sys.h"

static const uint8_t font[] =
{
    0x3F, //0
    0x06, //1
    0x5B, //2
    0x4F, //3
    0x66, //4
    0x6D, //5
    0x7D, //6
    0x07, //7
    0x7F, //8
    0x6F  //9
};

static void delay()
{
    esp_rom_delay_us(10);
}

static void start(tm1638_t *dev)
{
    gpio_set_level(dev->stb,0);
}

static void stop(tm1638_t *dev)
{
    gpio_set_level(dev->stb,1);
}

static void write_byte(tm1638_t *dev, uint8_t data)
{
    for(int i=0;i<8;i++)
    {
        gpio_set_level(dev->clk,0);

        gpio_set_level(dev->dio,data & 0x01);

        delay();

        gpio_set_level(dev->clk,1);

        delay();

        data >>=1;
    }
}

static void send_cmd(tm1638_t *dev,uint8_t cmd)
{
    start(dev);
    write_byte(dev,cmd);
    stop(dev);
}

void tm1638_init(tm1638_t *dev, gpio_num_t dio, gpio_num_t clk, gpio_num_t stb)
{
    dev->dio = dio;
    dev->clk = clk;
    dev->stb = stb;

    gpio_set_direction(dio,GPIO_MODE_OUTPUT);
    gpio_set_direction(clk,GPIO_MODE_OUTPUT);
    gpio_set_direction(stb,GPIO_MODE_OUTPUT);

    gpio_set_level(stb,1);
    gpio_set_level(clk,1);

    send_cmd(dev,0x8F); // display ON
}

void tm1638_write_digit(tm1638_t *dev,uint8_t pos,uint8_t seg)
{
    start(dev);
    write_byte(dev,0x44);
    stop(dev);

    start(dev);
    write_byte(dev,0xC0 + (pos<<1));
    write_byte(dev,seg);
    stop(dev);
}

void tm1638_display_raw(tm1638_t *dev, uint8_t *data)
{
    start(dev);

    write_byte(dev, 0x40);   // auto address mode

    stop(dev);

    start(dev);

    write_byte(dev, 0xC0);   // start address

    for(int i=0;i<16;i++)
        write_byte(dev, data[i]);

    stop(dev);
}

void tm1638_display_number(tm1638_t *dev, int num)
{
    for(int i = 0; i < 4; i++)
    {
       tm1638_write_digit(dev,i,font[num%10]);
        num /= 10;
    }
}