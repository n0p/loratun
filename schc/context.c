/**
 * \brief The database of the different rules used by the SCHC C/A
 * procedure.
 */

#include "context.h"
#include "schc.h"

/**
 * \brief The SCHC rules of the context used by the schc_compress() and
 * schc_decompress().
 *
 *
 * The schc_compress() and schc_decompress() functions will lookup the
 * information in these rules to determine how to apply
 * compression/decompression to a packet (if it matches).
 *
 * \note Because we use uint8_t type as Rule ID, we can have as many as
 * 255 rules. (Very unlikely for our purposes right now).
 *
 * \note The first rule that matches is the one which will be used to
 * compress. In the draft, the procedure should find the one which
 * matches and also offers the better Compression size, but in our
 * implementation, the first that matches is the one that is used. TODO
 * Think about how to implement this.
 *
 * \note The TV must be a valid string. Also, the format is not checked
 * before parsing, this is extremely dangerous in the case of
 * IPV6_DEV_PREFIX, IPV6_DEVIID, etc. Extreme caution must be taken when
 * writing the rules to make sure that the format is right.
 *
 * TODO It should not use a fixed size of [14], it should be dynamic
 * size of rows. Think about this. The ammount of rules should be
 * computed at runtime in the for() loops.
 */
struct field_description rules[][14] = {
	{ /* Rule 0 */

		/* All UDP packets with UDP sport == UDP dport == 8720. */

		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "123456789ABCDEFG", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "1b8ff9a3849bb317", EQUALS, NOT_SENT         },

		{ UDP_DEVPORT,         16, 1, BI, "10000",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "10000",            EQUALS, NOT_SENT         },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},
	{ /* Rule 1 */

		/* All UDP packets with UDP sport == UDP dport == 8720. */

		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "1b8ff9a3849bb317", EQUALS, NOT_SENT         },

		{ UDP_DEVPORT,         16, 1, BI, "10001",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "10001",            EQUALS, NOT_SENT         },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},

	{ /* Rule 2 */
		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "b1b2377fef871058", EQUALS, NOT_SENT         },

		{ UDP_DEVPORT,         16, 1, BI, "10002",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "10002",            EQUALS, NOT_SENT         },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},

	{ /* Rule 3 */
		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "0",                IGNORE, VALUE_SENT       },

		{ UDP_DEVPORT,         16, 1, BI, "10003",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "0",                IGNORE, VALUE_SENT       },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},

	{ /* Rule 4 */
		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "0",                IGNORE, VALUE_SENT       },
		{ IPV6_APPIID,         64, 1, BI, "0",                IGNORE, VALUE_SENT       },

		{ UDP_DEVPORT,         16, 1, BI, "10004",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "0",                IGNORE, VALUE_SENT       },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},

	{ /* Rule 5 */
		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "30f008da05cbe19a", EQUALS, NOT_SENT         },

		{ UDP_DEVPORT,         16, 1, BI, "10005",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "10005",            EQUALS, NOT_SENT         },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},
	{ /* Rule 6 */

		/* All UDP packets with UDP sport == UDP dport == 8720. */

		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                EQUALS, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               EQUALS, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "080027fffe000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FE80000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "1b8ff9a3849bb317", EQUALS, NOT_SENT         },

		{ UDP_DEVPORT,         16, 1, BI, "10006",            EQUALS, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "0",                IGNORE, VALUE_SENT       },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},
	{ /* Rule 7 */

		/* All UDP packets with UDP sport == UDP dport == 8720. */

		/* Field;               FL; FP;DI; TV;                 MO;     CA; */
		{ IPV6_VERSION,        4,  1, BI, "6",                EQUALS, NOT_SENT         },
		{ IPV6_TRAFFIC_CLASS,  8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_FLOW_LABEL,     20, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_PAYLOAD_LENGTH, 16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ IPV6_NEXT_HEADER,    8,  1, BI, "17",               IGNORE, NOT_SENT         },
		{ IPV6_HOP_LIMIT,      8,  1, BI, "0",                IGNORE, NOT_SENT         },
		{ IPV6_DEV_PREFIX,     64, 1, BI, "FD73AB4493DD6B18", EQUALS, NOT_SENT         },
		{ IPV6_DEVIID,         64, 1, BI, "0000000000000000", EQUALS, NOT_SENT         },
		{ IPV6_APP_PREFIX,     64, 1, BI, "FD73AB4493DD6B18", EQUALS, NOT_SENT         },
		{ IPV6_APPIID,         64, 1, BI, "0000000000000001", EQUALS, NOT_SENT         },

		{ UDP_DEVPORT,         16, 1, BI, "0",                IGNORE, NOT_SENT         },
		{ UDP_APPPORT,         16, 1, BI, "43690",            EQUALS, NOT_SENT         },
		{ UDP_LENGTH,          16, 1, BI, "0",                IGNORE, COMPUTE_LENGTH   },
		{ UDP_CHECKSUM,        16, 1, BI, "0",                IGNORE, COMPUTE_CHECKSUM }
	},
};
