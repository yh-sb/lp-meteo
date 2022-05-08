#ifndef INC_NMEA_PARSER_TYPES_H
#define INC_NMEA_PARSER_TYPES_H

#include "nmea.h"

typedef struct {
	nmea_t type;
	char type_word[3];
	nmea_s *data;
} nmea_parser_s;

#define NMEA_PARSER_PREFIX(parser, type_prefix) strncpy(parser->type_word, type_prefix, sizeof(parser->type_word))
#define NMEA_PARSER_TYPE(parser, nmea_type) parser->type = nmea_type

#endif  /* INC_NMEA_PARSER_TYPES_H */
