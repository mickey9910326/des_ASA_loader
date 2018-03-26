/**
 * @file pacdesc.h
 * @author LiYu
 * @date 2018.03.26
 * @brief Ddscriptions of ASAPROG packet.
 */

#ifndef PACDESC_H
#define PACDESC_H

/**
| NAME     | Bytes   | description |
| :--      | :--     | :--         |
| Header   |     3   | Always 0xFC,0xFC,0xFC |
| CmdType  |     1   | The command type of the packet. |
| DeviceID |     1   | The ID of device, only ASA_M128 now, it's 0x01. |
| Bytes    |     2   | The bytes of hex file. |
| DataBody | "Bytes" | Data of hex file. |
| Chksum   |     1   | The sum of each byte of DataBody. |
*/

#define HEADER   0xFC ///< The header of ASAPROG, will be 3 bytes
#define DEVICEID 0x01 ///< The ID of device, only ASA_M128 now, it's 0x01.

/**
 * @brief The command type of ASAPROG packet.
 */
enum cmdtype {
    CMD_CHKDEVICE = 0xFA, ///< Check the comport device is ASA device or not.
    CMD_RESCHK    = 0xFB, ///< Response of CMD_CHKDEVICE, if it's ASA_M128, it should response this type of packet.
    CMD_LODEHEX   = 0xFC, ///< Send hex of data to ASA_M128.
    CMD_RESDONE   = 0xFD  ///< If ASA_M128 load all hex will return this response.
};

/**
 * @brief The command type of ASAPROG packet.
 */
typedef enum cmdtype CmdType_t;

#endif // PACDESC_H
