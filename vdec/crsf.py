
import socket
import time
import struct

# Определите необходимые константы или импортируйте их из другого модуля
CRSF_BATTERY_SENSOR = 0x10  # Пример значения, замените на актуальную константу
CRSF_ATTITUDE_TYPE = 0x20
CRSF_FRAMETYPE_FLIGHT_MODE = 0x21

# Переменные для хранения состояний
crsf_battery_voltage = 0.0
crsf_battery_current = 0.0
crsf_pitch = 0.0
crsf_roll = 0.0
crsf_yaw = 0.0
crsf_flight_mode = ""

def crc8_dvb_s2(crc, a):
    crc ^= a
    for _ in range(8):
        if crc & 0x80:
            crc = (crc << 1) ^ 0xD5
        else:
            crc <<= 1
    return crc & 0xFF

def crc8_data(data):
    crc = 0
    for byte in data:
        crc = crc8_dvb_s2(crc, byte)
    return crc

def crsf_validate_frame(frame):
    if len(frame) < 4:
        return False
    calculated_crc = crc8_data(frame[2:-1])
    received_crc = frame[-1]
    return calculated_crc == received_crc

def handleCrsfPacket(ptype, data):
    global crsf_battery_voltage, crsf_battery_current, crsf_pitch, crsf_roll, crsf_yaw, crsf_flight_mode

    if ptype == CRSF_BATTERY_SENSOR:
        crsf_battery_voltage = struct.unpack_from(">H", data, 3)[0] / 10.0
        crsf_battery_current = struct.unpack_from(">H", data, 5)[0] / 10.0
    elif ptype == CRSF_ATTITUDE_TYPE:
        crsf_pitch = struct.unpack_from(">h", data, 3)[0] / 10000.0
        crsf_roll = struct.unpack_from(">h", data, 5)[0] / 10000.0
        crsf_yaw = struct.unpack_from(">h", data, 7)[0] / 10000.0
    elif ptype == CRSF_FRAMETYPE_FLIGHT_MODE:
        mode_length = data[1] - 3
        if 0 < mode_length < 256:
            crsf_flight_mode = data[3:3 + mode_length].decode('utf-8')
        else:
            crsf_flight_mode = "Unknown"

def crsf_thread(crsf_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    try:
        sock.bind(('0.0.0.0', crsf_port))
    except socket.error as e:
        print(f"Ошибка: не удалось привязать сокет CRSF: {e}")
        return

    buffer = bytearray(256)

    while True:
        try:
            len_received, _ = sock.recvfrom_into(buffer)
            if len_received > 0:
                expected_len = buffer[1] + 2
                if len_received >= expected_len:
                    if crsf_validate_frame(buffer[:len_received]):
                        handleCrsfPacket(buffer[2], buffer[:len_received])
                    else:
                        print("Ошибка CRC: пакет поврежден")
                else:
                    print("Ошибка: некорректная длина пакета")
        except socket.error as e:
            print(f"Ошибка при получении данных: {e}")

        time.sleep(0.001)  # Ожидание

    sock.close()

# Пример вызова
crsf_port = 12345  # Укажите порт для CRSF
crsf_thread(crsf_port)
