#if !defined(custom_uuid_h)
#define custom_uuid_h

#ifdef __cplusplus
    extern "C" {
#endif

/* The Company identified - TODO: must be changed*/

#define BT_COMPANY_ID_CODE      0x0059

/* Service UUID a3c87500-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_UUID                BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87500, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87501-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_CAPS_UUID           BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87501, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87502-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_SLOT_UUID           BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87502, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87503-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_INTV_UUID           BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87503, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87504-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_TX_UUID             BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87504, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87505-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_ADV_TX_UUID         BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87505, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87506-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_LOCK_UUID           BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87506, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87507-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_UNLOCK_UUID         BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87507, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87508-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_ECDH_UUID           BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87508, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c87509-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_EID_UUID            BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c87509, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c8750a-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_DATA_UUID           BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c8750a, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c8750b-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_RESET_UUID          BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c8750b, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));

/* Characteristic UUID a3c8750c-8ed3-4bdf-8a39-a01bebede295 */
#define EDS_CONNECTABLE_UUID    BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xa3c8750c, 0x8ed3, 0x4bdf, 0x8a39, 0xa01bebede295));


#ifdef __cplusplus
    }
#endif

#endif