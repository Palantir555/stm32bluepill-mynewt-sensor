#include <os/os.h>
#include <console/console.h>
#include <sensor/sensor.h>
#include <sensor/temperature.h>
#include "temp_sensor.h"

#define MY_SENSOR_POLL_TIME (10 * 1000)  //  Poll every 10,000 milliseconds (10 seconds)  
#define LISTENER_CB         1  //  This is a listener callback.
#define READ_CB             2  //  This is a sensor read callback.

static int read_temperature(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type);

static struct sensor *my_sensor;

static struct sensor_listener listener = {
    .sl_sensor_type = SENSOR_TYPE_AMBIENT_TEMPERATURE,
    .sl_func = read_temperature,
    .sl_arg = (void *) LISTENER_CB,
};

////  #if MYNEWT_VAL(TEMP_STM32_ONB) ////

    ///////////////////////////////////////////////////////////////////////////////
    //  Blue Pill Internal Temperature Sensor

    //  Initialise the Blue Pill internal temperature sensor that is connected to port ADC1 on channel 16.
    #include <adc_stm32f1/adc_stm32f1.h>
    #include <temp_stm32/temp_stm32.h>

    #define MY_SENSOR_DEVICE TEMP_STM32_DEVICE  //  Name of the internal temperature sensor: "temp_stm32_0"

    static int init_drivers(void) {
        //  Initialise the ADC1 port and channel 16 for the internal temperature sensor.

        //  Initialise the ADC1 port.
        stm32f1_adc_create();
        //  Initialise the internal temperature sensor channel on ADC1.
        temp_stm32_create();
        return 0;
    }

#if MYNEWT_VAL(TEMP_STM32_ONB) ////
#elif MYNEWT_VAL(BME280_OFB)

    ///////////////////////////////////////////////////////////////////////////////
    //  BME280 Temperature Sensor

    #define MY_SENSOR_DEVICE "bme280_0"          //  Name of the BME280 driver
    static int init_drivers(void) { return 0; }  //  BME280 driver already initialised by Sensor Creator.

#endif  //  MYNEWT_VAL(BME280_OFB)

int init_temperature_sensor(void) {
    //  Initialise the temperature sensor.  Poll the sensor every 10 seconds.
    int rc;

    //  Initialise the sensor drivers.
    init_drivers();

    //  Poll the sensor every 10 seconds.
    rc = sensor_set_poll_rate_ms(MY_SENSOR_DEVICE, MY_SENSOR_POLL_TIME);
    assert(rc == 0);

    //  Fetch the sensor.
    my_sensor = sensor_mgr_find_next_bydevname(MY_SENSOR_DEVICE, NULL);
    assert(my_sensor != NULL);

    //  Set the listener function to be called every 10 seconds.
    rc = sensor_register_listener(my_sensor, &listener);
    assert(rc == 0);
}

static int read_temperature(struct sensor* sensor, void *arg, void *databuf, sensor_type_t type) {
    //  This listener function is called every 10 seconds.  Mynewt has fetched the temperature data,
    //  passed through databuf.
    struct sensor_temp_data *temp;

    //  Check that the temperature data is valid.
    if (!databuf) { return SYS_EINVAL; }
    temp = (struct sensor_temp_data *)databuf;
    if (!temp->std_temp_is_valid) { return SYS_EINVAL; }

    //  Temperature data is valid.  Display it and send to server.
    console_printf(
        "temp = %d.%d\n",
        (int) (temp->std_temp),
        (int) (10.0 * temp->std_temp) % 10
    );
    return 0;
}