#ifndef ITCH_GOLDEN_PROTOCOL_H
#define ITCH_GOLDEN_PROTOCOL_H

#include <stdint.h>

// --------------------------------------------------------------------------------
// COMPILER CONFIGURATION (CRITICAL)
// --------------------------------------------------------------------------------
// We force 1-byte packing. If you miss this, the compiler will add "padding bytes"
// between fields to align them to RAM addresses, shifting your data by 1-3 bytes
// and corrupting the entire stream.
#pragma pack(push, 1)

// --------------------------------------------------------------------------------
// 1. PRIMITIVES & CONSTANTS
// --------------------------------------------------------------------------------

// NASDAQ Timestamps are 6 bytes (48-bit) nanoseconds from midnight.
// We treat them as a byte array to prevent endianness accidents during casting.
typedef struct {
    uint8_t t[6]; 
} itch_ts48_t;

// C23 constexpr for message sizes and types
constexpr int ITCH_HEADER_SIZE = 11;

// --------------------------------------------------------------------------------
// 2. COMMON HEADER (11 Bytes)
// --------------------------------------------------------------------------------
typedef struct {
    uint8_t     msg_type;       // The "Discriminator" (A, S, E, etc.)
    uint16_t    locate;         // Stock Locate ID (Big Endian)
    uint16_t    tracking;       // Internal Tracking Num (Big Endian)
    itch_ts48_t timestamp;      // Nanoseconds from midnight (Big Endian)
} itch_header_t;

// --------------------------------------------------------------------------------
// 3. SYSTEM & ADMIN MESSAGES
// --------------------------------------------------------------------------------

// Type 'S': System Event
// Used to signal Start of Day ('O'), End of Day ('C'), etc.
typedef struct {
    itch_header_t h;
    uint8_t event_code; 
} itch_msg_system_event_t;

// Type 'R': Stock Directory
// The "Rosetta Stone" that maps the Integer 'Locate' ID to the ASCII 'Stock' symbol.
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint8_t  market_category;
    uint8_t  financial_status;
    uint32_t round_lot_size;    // BE
    uint8_t  round_lots_only;
    uint8_t  issue_class;
    uint8_t  issue_subtype[2];
    uint8_t  authenticity;
    uint8_t  short_sale_threshold;
    uint8_t  ipo_flag;
    uint8_t  luld_ref_tier;
    uint8_t  etp_flag;
    uint32_t etp_leverage;      // BE
    uint8_t  inverse_indicator;
} itch_msg_stock_directory_t;

// Type 'H': Stock Trading Action
// Halts (Volatilty Pauses, LULD Pauses).
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint8_t  trading_state;     // 'H'=Halted, 'P'=Paused, 'Q'=Quotation Only, 'T'=Trading
    uint8_t  reserved;
    char     reason[4];
} itch_msg_trading_action_t;

// Type 'Y': Reg SHO Restriction
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint8_t  action;            // '0'=No Price Test, '1'=Reg SHO Short Sale Restriction
} itch_msg_reg_sho_t;

// Type 'L': Market Participant Position
typedef struct {
    itch_header_t h;
    char     mpid[4];
    char     stock[8];
    uint8_t  primary_market_maker;
    uint8_t  market_maker_mode;
    uint8_t  participant_state;
} itch_msg_participant_position_t;

// Type 'V': MWCB Decline Level (Market Wide Circuit Breaker)
typedef struct {
    itch_header_t h;
    uint64_t level1; // Price(8) - 8 decimals
    uint64_t level2; // Price(8)
    uint64_t level3; // Price(8)
} itch_msg_mwcb_decline_t;

// Type 'W': MWCB Status
typedef struct {
    itch_header_t h;
    uint8_t breached_level; // '1', '2', or '3'
} itch_msg_mwcb_status_t;

// Type 'K': IPO Quoting Period Update
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint32_t release_time;  // Seconds since midnight
    uint8_t  qualifier;     // 'A'=Anticipated, 'C'=Canceled
    uint32_t ipo_price;     // Price(4)
} itch_msg_ipo_quoting_t;

// Type 'J': LULD Auction Collar
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint32_t ref_price;     // Price(4)
    uint32_t upper_price;   // Price(4)
    uint32_t lower_price;   // Price(4)
    uint32_t extension;     // Price(4)
} itch_msg_luld_collar_t;

// Type 'h': Operational Halt (Note: Lowercase 'h')
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint8_t  market_code;
    uint8_t  action;
} itch_msg_operational_halt_t;

// Type 'N': Retail Price Improvement Indicator (RPII)
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint8_t  interest_flag; // 'B'=Bid, 'S'=Ask, 'A'=Both
} itch_msg_rpii_t;

// Type 'O': Direct Listing with Capital Raise (DLCR)
// ADDED: April 28, 2023.
typedef struct {
    itch_header_t h;
    char     stock[8];
    uint8_t  open_eligibility;      // 'O'=Open, 'D'=Delayed
    uint32_t min_allowable_price;   // Price(4)
    uint32_t max_allowable_price;   // Price(4)
    uint32_t near_execution_price;  // Price(4)
    uint64_t near_execution_time;   // 8 Bytes (Not 4!) - Nanoseconds
    uint32_t lower_collar;          // Price(4)
    uint32_t upper_collar;          // Price(4)
} itch_msg_dlcr_t;

// --------------------------------------------------------------------------------
// 4. ORDER BOOK MESSAGES
// --------------------------------------------------------------------------------

// Type 'A': Add Order (Anonymous)
typedef struct {
    itch_header_t h;
    uint64_t ref_num;       // The unique Order ID
    uint8_t  side;          // 'B'=Buy, 'S'=Sell
    uint32_t shares;
    char     stock[8];
    uint32_t price;         // Price(4)
} itch_msg_add_order_t;

// Type 'F': Add Order (Attributed)
typedef struct {
    itch_header_t h;
    uint64_t ref_num;
    uint8_t  side;
    uint32_t shares;
    char     stock[8];
    uint32_t price;         // Price(4)
    char     mpid[4];       // Market Participant ID
} itch_msg_add_order_mpid_t;

// Type 'E': Order Executed
typedef struct {
    itch_header_t h;
    uint64_t ref_num;
    uint32_t executed_shares;
    uint64_t match_num;     // The unique Trade ID
} itch_msg_executed_t;

// Type 'C': Order Executed With Price
typedef struct {
    itch_header_t h;
    uint64_t ref_num;
    uint32_t executed_shares;
    uint64_t match_num;
    uint8_t  printable;     // 'Y'=Print to tape, 'N'=Don't print
    uint32_t execution_price;// Price(4)
} itch_msg_executed_price_t;

// Type 'X': Order Cancel
typedef struct {
    itch_header_t h;
    uint64_t ref_num;
    uint32_t canceled_shares;
} itch_msg_cancel_t;

// Type 'D': Order Delete
typedef struct {
    itch_header_t h;
    uint64_t ref_num;
} itch_msg_delete_t;

// Type 'U': Order Replace
typedef struct {
    itch_header_t h;
    uint64_t original_ref_num;
    uint64_t new_ref_num;
    uint32_t shares;
    uint32_t price;         // Price(4)
} itch_msg_replace_t;

// --------------------------------------------------------------------------------
// 5. TRADE MESSAGES
// --------------------------------------------------------------------------------

// Type 'P': Trade (Non-Cross)
typedef struct {
    itch_header_t h;
    uint64_t ref_num;       // Usually 0 or internal
    uint8_t  side;          // 'B' (Always 'B' in newer specs, buy-side)
    uint32_t shares;
    char     stock[8];
    uint32_t price;         // Price(4)
    uint64_t match_num;
} itch_msg_trade_t;

// Type 'Q': Cross Trade (Open/Close/IPO)
typedef struct {
    itch_header_t h;
    uint32_t shares;
    char     stock[8];
    uint32_t cross_price;   // Price(4)
    uint64_t match_num;
    char     cross_type;    // 'O'=Open, 'C'=Close, 'H'=Halt/IPO, 'A'=Ext. Trading Close
} itch_msg_cross_trade_t;

// Type 'B': Broken Trade
typedef struct {
    itch_header_t h;
    uint64_t match_num;
} itch_msg_broken_trade_t;

// Type 'I': NOII (Net Order Imbalance)
typedef struct {
    itch_header_t h;
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    uint8_t  imbalance_direction;
    char     stock[8];
    uint32_t far_price;     // Price(4)
    uint32_t near_price;    // Price(4)
    uint32_t current_ref_price; // Price(4)
    char     cross_type;
    uint8_t  price_var_indicator;
} itch_msg_noii_t;


// --------------------------------------------------------------------------------
// 6. MASTER UNION (For the Reader Buffer)
// --------------------------------------------------------------------------------
typedef union {
    itch_header_t                   header;
    // Admin
    itch_msg_system_event_t         system_event;
    itch_msg_stock_directory_t      stock_directory;
    itch_msg_trading_action_t       trading_action;
    itch_msg_reg_sho_t              reg_sho;
    itch_msg_participant_position_t participant_position;
    itch_msg_mwcb_decline_t         mwcb_decline;
    itch_msg_mwcb_status_t          mwcb_status;
    itch_msg_ipo_quoting_t          ipo_quoting;
    itch_msg_luld_collar_t          luld_collar;
    itch_msg_operational_halt_t     operational_halt;
    itch_msg_rpii_t                 rpii;
    itch_msg_dlcr_t                 dlcr;
    // Orders
    itch_msg_add_order_t            add_order;
    itch_msg_add_order_mpid_t       add_order_mpid;
    itch_msg_executed_t             executed;
    itch_msg_executed_price_t       executed_price;
    itch_msg_cancel_t               cancel;
    itch_msg_delete_t               delete;
    itch_msg_replace_t              replace;
    // Trades
    itch_msg_trade_t                trade;
    itch_msg_cross_trade_t          cross_trade;
    itch_msg_broken_trade_t         broken_trade;
    itch_msg_noii_t                 noii;
    
    // Max size of any message is < 64 bytes.
    // We add a safety buffer to 64 bytes.
    uint8_t raw[64]; 
} itch_message_t;

// Restore default packing
#pragma pack(pop)

#endif // ITCH_GOLDEN_PROTOCOL_H
