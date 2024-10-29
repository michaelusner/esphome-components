#include "pentair_easytouch_component.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esphome/core/log.h"

#ifdef ESPHOME_LOG_LEVEL
#undef ESPHOME_LOG_LEVEL
#endif
#define ESPHOME_LOG_LEVEL ESPHOME_LOG_LEVEL_VERBOSE
namespace esphome
{
    namespace pentair_easytouch
    {
        static const char *TAG = "pentair_easytouch.component";
        static const char *FEATURE_NAMES[] = {
            "Unknown",
            "Spa",
            "Cleaner",
            "Air Blower",
            "Spa Light",
            "Pool Light",
            "Pool",
            "Water Feature",
            "Spillway",
            "Aux"};

        void PentairEasyTouchComponent::setup()
        {
            if (this->flow_control_pin_ != nullptr)
            {
                this->flow_control_pin_->setup();
                this->flow_control_pin_->digital_write(0);
            }
            // Initialize all feature states to false
            memset(feature_states_, 0, sizeof(feature_states_));
        }

        uint8_t PentairEasyTouchComponent::calculate_checksum(uint8_t *packet, size_t length)
        {
            uint8_t checksum = 0;
            for (size_t i = 0; i < length; i++)
            {
                checksum += packet[i];
            }
            return checksum;
        }

        void PentairEasyTouchComponent::send_control_packet(FEATURE feature, bool state)
        {
            if (this->flow_control_pin_ != nullptr)
            {
                this->flow_control_pin_->digital_write(1); // Enable RS485 transmitter
            }

            uint8_t state_byte = state ? 0x01 : 0x00;
            uint8_t packet[] = {
                0xFF,       // Start of packet
                0x00,       // Start of packet
                0xA5,       // Start of packet
                0x10,       // Control command
                MAIN,       // Destination address (main panel)
                REMOTE,     // Source address (pretend to be a remote)
                0x86,       // Command (circuit on/off)
                0x02,       // Length
                state_byte, // State
                0x00        // Placeholder for checksum
            };

            packet[9] = calculate_checksum(packet, 9);

            ESP_LOGV(TAG, "Sending control packet for %s: %s",
                     FEATURE_NAMES[feature],
                     state ? "ON" : "OFF");

            this->write_array(packet, sizeof(packet));
            this->flush();

            // Store the new state
            if (feature < sizeof(feature_states_))
            {
                feature_states_[feature] = state;
            }

            if (this->flow_control_pin_ != nullptr)
            {
                this->flow_control_pin_->digital_write(0); // Disable RS485 transmitter
            }
        }

        bool PentairEasyTouchComponent::get_feature_state(FEATURE feature) const
        {
            if (feature < sizeof(feature_states_))
            {
                return feature_states_[feature];
            }
            return false;
        }

        void PentairEasyTouchComponent::update_feature_states()
        {
            // Update feature states from broadcast packet
            // equip1, equip2, and equip3 contain circuit states as bits
            feature_states_[POOL] = (broadcast_packet.equip1 & (1 << 0)) != 0;
            feature_states_[SPA] = (broadcast_packet.equip1 & (1 << 1)) != 0;
            feature_states_[CLEANER] = (broadcast_packet.equip1 & (1 << 2)) != 0;
            feature_states_[AIR_BLOWER] = (broadcast_packet.equip1 & (1 << 3)) != 0;
            feature_states_[SPA_LIGHT] = (broadcast_packet.equip1 & (1 << 4)) != 0;
            feature_states_[POOL_LIGHT] = (broadcast_packet.equip1 & (1 << 5)) != 0;
            feature_states_[WATER_FEATURE] = (broadcast_packet.equip2 & (1 << 0)) != 0;
            feature_states_[SPILLWAY] = (broadcast_packet.equip2 & (1 << 1)) != 0;
        }

        void PentairEasyTouchComponent::handle_status_packet()
        {

            if (this->packet_header.command == STATUS_PACKET)
            {
                ESP_LOGV(TAG, "Received status packet");
            }
            else
            {
                ESP_LOGW(TAG, "Received unknown packet type: %02X", this->packet_header.command);
                ESP_LOGD(TAG, "Packet: %02X %02X %02X %02X %02X %02X",
                         this->packet_header.leading_byte,
                         this->packet_header.type,
                         this->packet_header.dest,
                         this->packet_header.src,
                         this->packet_header.command,
                         this->packet_header.length);
                ESP_LOGD(TAG, "Broadcast Packet: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X, ",
                         this->broadcast_packet.hour,
                         this->broadcast_packet.minute,
                         this->broadcast_packet.equip1,
                         this->broadcast_packet.equip2,
                         this->broadcast_packet.equip3,
                         this->broadcast_packet.reserved1,
                         this->broadcast_packet.reserved2,
                         this->broadcast_packet.reserved3,
                         this->broadcast_packet.reserved4,
                         this->broadcast_packet.uom,
                         this->broadcast_packet.valve,
                         this->broadcast_packet.reserved5,
                         this->broadcast_packet.delay,
                         this->broadcast_packet.unknown,
                         this->broadcast_packet.pool_temp,
                         this->broadcast_packet.water_temp,
                         this->broadcast_packet.heater_active,
                         this->broadcast_packet.reserved6,
                         this->broadcast_packet.air_temp,
                         this->broadcast_packet.solar_temp,
                         this->broadcast_packet.reserved7,
                         this->broadcast_packet.reserved8,
                         this->broadcast_packet.heater_mode,
                         this->broadcast_packet.reserved9,
                         this->broadcast_packet.reserved10,
                         this->broadcast_packet.reserved11,
                         this->broadcast_packet.misc2);
                return;
            }
            // Update all feature states
            this->update_feature_states();

            // Update sensor values
            if (this->air_temperature_sensor_ != nullptr)
                this->air_temperature_sensor_->publish_state(this->broadcast_packet.air_temp);
            if (this->water_temperature_sensor_ != nullptr)
                this->water_temperature_sensor_->publish_state(this->broadcast_packet.water_temp);
            ESP_LOGV(TAG, "Status Update:");
            ESP_LOGV(TAG, "  Temperatures: Air=%d°F, Water=%d°F, Pool=%d°F",
                     this->broadcast_packet.air_temp,
                     this->broadcast_packet.water_temp,
                     this->broadcast_packet.pool_temp);
            ESP_LOGV(TAG, "  Heater: Active=0x%02X, Mode=0x%02X",
                     this->broadcast_packet.heater_active,
                     this->broadcast_packet.heater_mode);
            ESP_LOGV(TAG, "  Equipment: %02X %02X %02X",
                     this->broadcast_packet.equip1,
                     this->broadcast_packet.equip2,
                     this->broadcast_packet.equip3);
        }

        void PentairEasyTouchComponent::update()
        {
            while (this->available())
            {
                switch (this->read_state)
                {
                case READING_CHARS:
                    this->read_byte(&this->ch);
                    if (this->last == 0xff && this->ch == 0xA5)
                    {
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
                        if (this->packet_header.type == STATUS_PACKET &&
                            this->packet_header.src == MAIN &&
                            this->packet_header.dest == BROADCAST)
                        {
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
                    ESP_LOGD(TAG, "Received broadcast packet");
                    this->handle_status_packet();
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
            ESP_LOGCONFIG(TAG, "Pentair EasyTouch Component");
            LOG_SENSOR("", "Air Temperature", this->air_temperature_sensor_);
            LOG_SENSOR("", "Water Temperature", this->water_temperature_sensor_);
        }

        void UARTSwitch::dump_config()
        {
            LOG_SWITCH("", "UART Switch", this);
            ESP_LOGCONFIG(TAG, "  Feature: %s", FEATURE_NAMES[this->feature_]);
        }

        void UARTSwitch::write_state(bool state)
        {
            ESP_LOGV(TAG, "Setting %s to %s", FEATURE_NAMES[this->feature_], state ? "ON" : "OFF");
            this->parent_->send_control_packet(this->feature_, state);
            this->publish_state(state);
        }

    } // namespace pentair_easytouch
} // namespace esphome