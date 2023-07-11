#include "pentair_easytouch_component.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome
{
    namespace pentair_easytouch
    {
        static const char *TAG = "pentair_easytouch.component";

        void Demo_Task(void *arg)
        {
            PentairEasyTouchComponent *easytouch = (PentairEasyTouchComponent *)arg;
            while (1)
            {
                ESP_LOGV(TAG, "Demo_Task: %d", easytouch->available());
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
        }

        void PentairEasyTouchComponent::setup()
        {
            if (this->flow_control_pin_ != nullptr)
            {
                this->flow_control_pin_->setup();
                this->flow_control_pin_->digital_write(0);
            }
            // xTaskCreate(Demo_Task, "Demo_Task", 4096, NULL, 10, NULL);
        }

        void PentairEasyTouchComponent::update()
        {
            while (this->available())
            {
                switch (this->read_state)
                {
                case READING_CHARS:
                    this->read_byte(&this->ch);
                    // ESP_LOGV(TAG, "%d (0X%x)", this->ch, this->ch);
                    if (this->last == 0xff && this->ch == 0xA5)
                    {
                        // ESP_LOGV(TAG, "Packet start");
                        this->last = 0x00;
                        this->read_state = READING_PACKET_HEADER;
                        this->bytes_read = 1;
                        return;
                    }
                    else
                        this->last = this->ch;
                    break;
                case READING_PACKET_HEADER:
                    this->read_byte((uint8_t *)(&this->packet_header) + this->bytes_read++);
                    if (this->bytes_read >= sizeof(PacketHeader))
                    {
                        ESP_LOGV(TAG, "leading_byte = 0x%02x\ntype         = 0x%02x\ndest         = 0x%02x\nsrc          = 0x%02x\ncommand      = 0x%02x\nlength       = 0x%02x\n",
                                 this->packet_header.leading_byte,
                                 this->packet_header.type,
                                 this->packet_header.dest,
                                 this->packet_header.src,
                                 this->packet_header.command,
                                 this->packet_header.length);
                        if (this->packet_header.type == STATUS_PACKET && this->packet_header.src == CONTROLLER && this->packet_header.dest == BROADCAST)
                        {
                            ESP_LOGV(TAG, "Reading packet body");
                            this->read_state = READING_PACKET_BODY;
                        }
                        else
                        {
                            this->read_state = READING_CHARS;
                            this->bytes_read = 0;
                            return;
                        }
                    }
                    break;
                case READING_PACKET_BODY:
                    this->read_array((uint8_t *)&this->broadcast_packet, this->packet_header.length);
                    ESP_LOGV(TAG, "\nHour: %d\nMinute: %d\nAir temp: %d\nWater temp: %d\n",
                             this->broadcast_packet.hour,
                             this->broadcast_packet.minute,
                             this->broadcast_packet.air_temp,
                             this->broadcast_packet.water_temp);

                    if (this->air_temperature_sensor_ != nullptr)
                        this->air_temperature_sensor_->publish_state(this->broadcast_packet.air_temp);
                    if (this->water_temperature_sensor_ != nullptr)
                        this->water_temperature_sensor_->publish_state(this->broadcast_packet.water_temp);
                    // if (this->pump1_switch_ != nullptr)
                    //     this->pump1_switch_->write_state(true);
                    this->read_state = READING_CHARS;
                    this->bytes_read = 0;
                    break;
                default:
                    break;
                }
            }
        }

        void PentairEasyTouchComponent::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Pentair EasyTouch component");
            LOG_SENSOR(TAG, "Air Temp: ", this->air_temperature_sensor_);
        }

        void PentairEasyTouchComponent::write_binary(bool state)
        {
            this->write_str(ONOFF(state));
        }

        void UARTSwitch::dump_config()
        {
            LOG_SWITCH("", "UART Switch", this);
        }

        void UARTSwitch::write_state(bool state)
        {
            ESP_LOGV(TAG, "UARTSwitch::write_state(%d) Address=%02x", state, this->address_);
            this->parent_->write_binary(state);
            this->publish_state(state);
        }

    } // pentair_easytouch
} // esphome