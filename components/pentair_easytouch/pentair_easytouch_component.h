#pragma once
#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

#define BUFFER_SIZE 128

typedef enum
{
    READING_CHARS = 0,
    READING_PACKET_HEADER = 1,
    READING_PACKET_BODY = 2
} ReadState;

typedef enum
{
    STATUS_PACKET = 0x02,
    COMMAND_PACKET = 0x14,
    PUMP_PACKET = 0x0F,
    HEATER_PACKET = 0x20
} PACKET_TYPE;

typedef enum
{
    CHLORINATOR = 0x02,
    BROADCAST = 0x0f,
    MAIN = 0x10,
    SECONDARY = 0x11,
    REMOTE = 0x20,
    SCREENLOGIC = 0x22,
    PUMP1 = 0x60,
    PUMP2 = 0x61,
    PUMP3 = 0x62,
    PUMP4 = 0x63
} DEVICE;

typedef enum
{
    UNKNOWN = 0,
    SPA = 1,
    CLEANER = 2,
    AIR_BLOWER = 3,
    SPA_LIGHT = 4,
    POOL_LIGHT = 5,
    POOL = 6,
    WATER_FEATURE = 7,
    SPILLWAY = 8,
    AUX = 9
} FEATURE;

// Explicit cast operator for int to FEATURE
inline FEATURE int_to_feature(int value)
{
    return static_cast<FEATURE>(value);
}

typedef struct
{
    unsigned char leading_byte;
    unsigned char type;
    unsigned char dest;
    unsigned char src;
    unsigned char command;
    unsigned char length;
} PacketHeader;

typedef struct
{
    unsigned char hour;          // 6
    unsigned char minute;        // 7
    unsigned char equip1;        // 8  - Circuit states (bits)
    unsigned char equip2;        // 9  - More circuit states
    unsigned char equip3;        // 10 - More circuit states
    unsigned char reserved1;     // 11
    unsigned char reserved2;     // 12
    unsigned char reserved3;     // 13
    unsigned char reserved4;     // 14
    unsigned char uom;           // 15 - 0 == Fahrenheit, 4 == celsius
    unsigned char valve;         // 16
    unsigned char reserved5;     // 17
    unsigned char delay;         // 18
    unsigned char unknown;       // 19
    unsigned char pool_temp;     // 20
    unsigned char water_temp;    // 21
    unsigned char heater_active; // 22 - bit 0: pool heater, bit 1: spa heater
    unsigned char reserved6;     // 23
    unsigned char air_temp;      // 24
    unsigned char solar_temp;    // 25
    unsigned char reserved7;     // 26
    unsigned char reserved8;     // 27
    unsigned char heater_mode;   // 28
    unsigned char reserved9;     // 29
    unsigned char reserved10;    // 30
    unsigned char reserved11;    // 31
    unsigned char misc2;         // 32
} BroadcastPacket;

namespace esphome
{
    namespace pentair_easytouch
    {
        class PentairEasyTouchComponent : public PollingComponent, public uart::UARTDevice
        {
        public:
            PentairEasyTouchComponent() : PollingComponent(50) {}
            float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }
            void setup() override;
            void update() override;
            void dump_config() override;

            void set_air_temperature_sensor(sensor::Sensor *sensor) { air_temperature_sensor_ = sensor; }
            void set_water_temperature_sensor(sensor::Sensor *sensor) { water_temperature_sensor_ = sensor; }
            void set_flow_control_pin(GPIOPin *flow_control_pin) { this->flow_control_pin_ = flow_control_pin; }

            void send_control_packet(FEATURE feature, bool state);
            uint8_t calculate_checksum(uint8_t *packet, size_t length);
            bool get_feature_state(FEATURE feature) const;

            uint8_t buffer[BUFFER_SIZE];
            uint8_t *p_buffer = buffer;
            int bytes_read = 0;
            ReadState read_state = READING_CHARS;
            PacketHeader packet_header;
            BroadcastPacket broadcast_packet;
            GPIOPin *flow_control_pin_{nullptr};

        protected:
            sensor::Sensor *air_temperature_sensor_{nullptr};
            sensor::Sensor *water_temperature_sensor_{nullptr};
            uint8_t ch, last = 0x00;

            // Store current states for all features
            bool feature_states_[16]{false}; // Store states for up to 16 features
            void update_feature_states();
            void handle_status_packet();
        };

        class UARTSwitch : public Component, public switch_::Switch
        {
        public:
            void dump_config() override;
            void set_parent(PentairEasyTouchComponent *parent) { this->parent_ = parent; }
            void set_feature(int feature) { this->feature_ = int_to_feature(feature); }
            FEATURE get_feature() const { return feature_; }

        protected:
            void write_state(bool state) override;
            PentairEasyTouchComponent *parent_;
            FEATURE feature_;
        };

    } // namespace pentair_easytouch
} // namespace esphome