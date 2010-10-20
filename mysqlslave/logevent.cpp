/*
 * logevent.cpp
 *
 *  Created on: 28.09.2010
 *      Author: azrahel
 */

#include "logevent.h"

namespace mysql {

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

CLogEvent::CLogEvent()
{
}

CLogEvent::~CLogEvent() throw()
{
}

int CLogEvent::tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt) 
{
	_when = uint4korr(data);
	_server_id = uint4korr(data + SERVER_ID_OFFSET);
	_data_written = uint4korr(data + EVENT_LEN_OFFSET);
	_log_pos= uint4korr(data + LOG_POS_OFFSET);
	_flags = uint2korr(data + FLAGS_OFFSET);
	
	return 0;
}


int CLogEvent::Info(uint8_t* buf, size_t event_len)
{
	if (event_len < EVENT_LEN_OFFSET || event_len != uint4korr(buf+EVENT_LEN_OFFSET))
	{
		//_last_error = "Sanity check failed";		// Needed to free buffer
		return -1; // general sanity check - will fail on a partial read
	}

	printf("event type: %d\n", buf[EVENT_TYPE_OFFSET]);

	return 0;
}





/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


CFormatDescriptionLogEvent::CFormatDescriptionLogEvent(uint8_t binlog_ver, const char* server_ver)
{
	_binlog_version= binlog_ver;
	 if( _binlog_version == 4 )
	 {
		 if( server_ver )
			 memcpy(_server_version, server_ver, ST_SERVER_VER_LEN);
		 else
			 _server_version[0] = '0';
		_common_header_len = LOG_EVENT_HEADER_LEN;
		_number_of_event_types = LOG_EVENT_TYPES;

		_post_header_len = new uint8_t[_number_of_event_types];
		/*
		  This long list of assignments is not beautiful, but I see no way to
		  make it nicer, as the right members are #defines, not array members, so
		  it's impossible to write a loop.
		*/
		if( _post_header_len )
		{
			/* Note: all event types must explicitly fill in their lengths here. */
			_post_header_len[START_EVENT_V3-1]= START_V3_HEADER_LEN;
			_post_header_len[QUERY_EVENT-1]= QUERY_HEADER_LEN;
			_post_header_len[STOP_EVENT-1]= STOP_HEADER_LEN;
			_post_header_len[ROTATE_EVENT-1]= ROTATE_HEADER_LEN;
			_post_header_len[INTVAR_EVENT-1]= INTVAR_HEADER_LEN;
			_post_header_len[LOAD_EVENT-1]= LOAD_HEADER_LEN;
			_post_header_len[SLAVE_EVENT-1]= SLAVE_HEADER_LEN;
			_post_header_len[CREATE_FILE_EVENT-1]= CREATE_FILE_HEADER_LEN;
			_post_header_len[APPEND_BLOCK_EVENT-1]= APPEND_BLOCK_HEADER_LEN;
			_post_header_len[EXEC_LOAD_EVENT-1]= EXEC_LOAD_HEADER_LEN;
			_post_header_len[DELETE_FILE_EVENT-1]= DELETE_FILE_HEADER_LEN;
			_post_header_len[NEW_LOAD_EVENT-1]= NEW_LOAD_HEADER_LEN;
			_post_header_len[RAND_EVENT-1]= RAND_HEADER_LEN;
			_post_header_len[USER_VAR_EVENT-1]= USER_VAR_HEADER_LEN;
			_post_header_len[FORMAT_DESCRIPTION_EVENT-1]= FORMAT_DESCRIPTION_HEADER_LEN;
			_post_header_len[XID_EVENT-1]= XID_HEADER_LEN;
			_post_header_len[BEGIN_LOAD_QUERY_EVENT-1]= BEGIN_LOAD_QUERY_HEADER_LEN;
			_post_header_len[EXECUTE_LOAD_QUERY_EVENT-1]= EXECUTE_LOAD_QUERY_HEADER_LEN;
			/*
			The PRE_GA events are never be written to any binlog, but
			their lengths are included in Format_description_log_event.
			Hence, we need to be assign some value here, to avoid reading
			uninitialized memory when the array is written to disk.
			*/
			_post_header_len[PRE_GA_WRITE_ROWS_EVENT-1] = 0;
			_post_header_len[PRE_GA_UPDATE_ROWS_EVENT-1] = 0;
			_post_header_len[PRE_GA_DELETE_ROWS_EVENT-1] = 0;

			_post_header_len[TABLE_MAP_EVENT-1]=    TABLE_MAP_HEADER_LEN;
			_post_header_len[WRITE_ROWS_EVENT-1]=   ROWS_HEADER_LEN;
			_post_header_len[UPDATE_ROWS_EVENT-1]=  ROWS_HEADER_LEN;
			_post_header_len[DELETE_ROWS_EVENT-1]=  ROWS_HEADER_LEN;


			_post_header_len[INCIDENT_EVENT-1]= INCIDENT_HEADER_LEN;

			calc_server_version_split();
		}
	 }
	 else
	 {
		 delete [] _post_header_len;
		 _post_header_len = NULL;
	 }

}

CFormatDescriptionLogEvent::~CFormatDescriptionLogEvent() throw()
{
	if( _post_header_len )
	{
		delete [] _post_header_len;
		_post_header_len = NULL;
	}
}

void CFormatDescriptionLogEvent::calc_server_version_split()
{
	if( _server_version[0] )
	{
		char *p = _server_version, *r;
		unsigned long number;
		for( int i= 0; i<=2; i++)
		{
			number= strtoul(p, &r, 10);
			_server_version_split[i]= (uint8_t)number;
			p = r;
			if (*r == '.')
			  p++; // skip the dot
		}
	}
	else
		memset(_server_version_split, 0x00, 3);
}



/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int CIntvarLogEvent::tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt) 
{
	CLogEvent::tune(data, size, fmt);
	data += fmt->_common_header_len + fmt->_post_header_len[INTVAR_EVENT-1];
	_type= data[I_TYPE_OFFSET];
	_val= uint8korr(data+I_VAL_OFFSET);
	return 0;
}



/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

int CQueryLogEvent::tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt)
{
	uint8_t common_header_len, post_header_len;
	uint64_t data_len;

	_query[0] = '\0';

	common_header_len = fmt->_common_header_len;
	post_header_len= fmt->_post_header_len[QUERY_EVENT-1];

	data_len = size - (common_header_len + post_header_len);
	data += common_header_len;

	_exec_time = uint4korr(data + Q_EXEC_TIME_OFFSET);

	_db_len = (uint32_t)data[Q_DB_LEN_OFFSET];
	_error_code = uint2korr(data + Q_ERR_CODE_OFFSET);

	if (post_header_len - QUERY_HEADER_MINIMAL_LEN)
	{
		_status_vars_len= uint2korr(data + Q_STATUS_VARS_LEN_OFFSET);
		if (_status_vars_len > data_len || _status_vars_len > MAX_SIZE_LOG_EVENT_STATUS) return -1;

		data_len -= _status_vars_len;
	}
	else
	{
		return -1;
	}

	// смещаемся к query
	data += post_header_len + _status_vars_len + _db_len + 1;
	data_len -= (_db_len + 1);
	_q_len = data_len;

	memcpy(_query, data, _q_len);
	_query[_q_len] = '\0';

	return 0;
}


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
uint64_t CTableMapLogEvent::get_table_id(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt)
{
	uint8_t *post_start = data + fmt->_common_header_len + TM_MAPID_OFFSET;
	return fmt->_post_header_len[TABLE_MAP_EVENT-1] == 6 ?
			(uint64_t)uint4korr(post_start) : (uint64_t)uint6korr(post_start);
}

const char* CTableMapLogEvent::get_database_name(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt)
{
	return (const char*)(data + fmt->_common_header_len + fmt->_post_header_len[TABLE_MAP_EVENT-1] + 1);
}

const char* CTableMapLogEvent::get_table_name(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt)
{
	uint8_t *p = data + fmt->_common_header_len + fmt->_post_header_len[TABLE_MAP_EVENT-1];
	return (const char*)(p + *p + 3);
}

CTableMapLogEvent::CTableMapLogEvent()
	: _table_id(0)
	, _data(NULL)
	, _size(0)
	, _column_count(0)
	, _metadata(NULL)
{
	_db_name[0] = '\0';
	_table_name[0] = '\0';
}

// CTableMapLogEvent::CTableMapLogEvent(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt)
// {
// 	_size = size;
// 	_data = new uint8_t[_size];
// 	memcpy(_data, data, _size);
// 	_table_id = get_table_id(data, size, fmt);
// }

CTableMapLogEvent::~CTableMapLogEvent() throw()
{
	if( _data )
		delete [] _data;
}
const char * CTableMapLogEvent::get_database_name() const
{
	return _db_name;
}

const char * CTableMapLogEvent::get_table_name() const
{
	return _table_name;
}

int CTableMapLogEvent::get_column_count() const
{
	return _column_count;
}

int CTableMapLogEvent::tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt)
{
	CLogEvent::tune(data, size, fmt);
	if( _data )
		delete [] _data;
	_size = size;
	_data = new uint8_t[_size];
	memcpy(_data, data, _size);

	_table_id = get_table_id(_data, size, fmt);
	
	uint8_t *p;
	uint8_t len = 0;
	
	// db name
	p = _data + fmt->_common_header_len + fmt->_post_header_len[TABLE_MAP_EVENT-1];
	len = *p++;
	strcpy(_db_name, (const char*)p);
	p += len + 1;
	
	// table name
	len = *p++;
	strcpy(_table_name, (const char*)p);
	p += len + 1;
	
	// column count
	_column_count = net_field_length(&p);
	_column_types = p;
	p += _column_count;
	
	// metadata ptr
	_metadata_length = net_field_length(&p);
	_metadata = p;
	return 0;
}


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int CRowLogEvent::tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt) 
{
	const uint8_t *p;
	
	CLogEvent::tune(data, size, fmt);
	_valid = 0;
	_type = data[EVENT_TYPE_OFFSET];
	data += fmt->_common_header_len;
	size -= fmt->_common_header_len;
	p = data;
	
	p += RW_MAPID_OFFSET;
	_table_id= (uint64_t)uint6korr(p);
	
	p += RW_FLAGS_OFFSET;
	_row_flags= uint2korr(p);
	
	p+=2;
	_ncolumns = net_field_length((u_char**)&p);
	
	_used_columns_mask = build_column_mask(&p, NULL, _ncolumns);
	if( _used_columns_mask == (uint64_t) -1 )
		return -1;
	update_n1bits(&_used_columns_mask);
	
	if( _type == UPDATE_ROWS_EVENT )
	{
		_used_columns_afterimage_mask = build_column_mask(&p, NULL, _ncolumns);
		if( _used_columns_afterimage_mask == (uint64_t) -1 )
			return -1;
		update_n1bits(&_used_columns_afterimage_mask);
	}
	
	_len = size - (p - data);
	_data = p;
	_valid = 1;

	return 0;
}

void CRowLogEvent::update_n1bits(uint64_t *mask)
{
	uint64_t m = *mask;
	uint8_t nbits = 0;
	do
	{
		if( m & 0x01 )
			nbits++;
	}
	while( m>>=1 );
	
	*mask = (*mask & 0x00FFFFFFFFFFFFFF) | ((uint64_t)nbits << (64-8));
}

uint64_t CRowLogEvent::build_column_mask(const uint8_t **ptr, size_t *len, uint64_t n)
{
	uint64_t column_mask;
	uint8_t l = uint8_t((n+7)/8);
	
	switch( l )
	{
	case 1: // [1..8] columns
	{
		column_mask = 0xFFFFFFFFFFFFFFFF & (uint64_t)*(*ptr);
		break;
	}
	case 2: // [9..16] columns
	{
		column_mask = 0xFFFFFFFFFFFFFFFF & (uint64_t)uint2korr(*ptr);
		break;
	}
	case 3: // [17..24] columns
	{
		column_mask = 0xFFFFFFFFFFFFFFFF & (uint64_t)uint3korr(*ptr);
		break;
	}
	case 4: // [25..32] columns
	{
		column_mask = 0xFFFFFFFFFFFFFFFF & (uint64_t)uint4korr(*ptr);
		break;
	}
	case 5: // [33..40] columns
	{
		column_mask = 0xFFFFFFFFFFFFFFFF & (uint64_t)uint5korr(*ptr);
		break;
	}
	case 6: // [41..48] columns
	{
		column_mask = 0xFFFFFFFFFFFFFFFF & (uint64_t)uint6korr(*ptr);
		break;
	}
	default:
	{
		// wrong column number
		return (uint64_t)-1;
	}
	}
	
	if( ptr && *ptr )
		(*ptr) += l; 
	
	if( len )
		*len -= l;
	
	return column_mask;
}

}
