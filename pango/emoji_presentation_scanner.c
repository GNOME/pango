// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


static const char _emoji_presentation_actions[] = {
	0, 1, 0, 1, 1, 1, 5, 1,
	6, 1, 7, 1, 8, 1, 9, 2,
	2, 3, 2, 2, 4, 0
};

static const char _emoji_presentation_key_offsets[] = {
	0, 3, 8, 9, 13, 15, 22, 26,
	33, 42, 52, 63, 71, 82, 92, 103,
	115, 116, 121, 0
};

static const unsigned char _emoji_presentation_trans_keys[] = {
	9u, 10u, 12u, 3u, 7u, 13u, 0u, 2u,
	6u, 10u, 12u, 8u, 9u, 14u, 15u, 2u,
	3u, 6u, 7u, 13u, 0u, 1u, 9u, 10u,
	11u, 12u, 2u, 3u, 6u, 7u, 13u, 0u,
	1u, 2u, 3u, 6u, 7u, 10u, 12u, 13u,
	0u, 1u, 2u, 3u, 6u, 7u, 9u, 10u,
	12u, 13u, 0u, 1u, 2u, 3u, 4u, 6u,
	7u, 9u, 10u, 12u, 13u, 0u, 1u, 2u,
	3u, 6u, 7u, 10u, 13u, 0u, 1u, 2u,
	3u, 6u, 7u, 9u, 10u, 12u, 13u, 14u,
	0u, 1u, 2u, 3u, 4u, 6u, 7u, 10u,
	12u, 13u, 0u, 1u, 2u, 3u, 6u, 7u,
	9u, 10u, 11u, 12u, 13u, 0u, 1u, 2u,
	3u, 4u, 6u, 7u, 9u, 10u, 11u, 12u,
	13u, 0u, 1u, 6u, 10u, 11u, 12u, 8u,
	9u, 2u, 3u, 6u, 7u, 9u, 10u, 11u,
	12u, 13u, 14u, 0u, 1u, 0u
};

static const char _emoji_presentation_single_lengths[] = {
	3, 3, 1, 2, 2, 5, 4, 5,
	7, 8, 9, 6, 9, 8, 9, 10,
	1, 3, 10, 0
};

static const char _emoji_presentation_range_lengths[] = {
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 0
};

static const char _emoji_presentation_index_offsets[] = {
	0, 4, 9, 11, 15, 18, 25, 30,
	37, 46, 56, 67, 75, 86, 96, 107,
	119, 121, 126, 0
};

static const char _emoji_presentation_trans_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, 0
};

static const short _emoji_presentation_trans_offsets[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87,
	88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 155, 0
};

static const char _emoji_presentation_trans_lengths[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 0
};

static const char _emoji_presentation_cond_keys[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

static const char _emoji_presentation_cond_targs[] = {
	7, 1, 11, 5, 13, 8, 8, 8,
	5, 7, 5, 1, 11, 7, 5, 4,
	7, 5, 14, 15, 16, 17, 18, 6,
	5, 7, 1, 5, 11, 5, 9, 10,
	2, 3, 12, 0, 5, 9, 10, 2,
	3, 1, 11, 12, 0, 5, 9, 10,
	2, 3, 7, 1, 11, 12, 0, 5,
	9, 10, 11, 2, 3, 7, 1, 11,
	12, 0, 5, 9, 10, 2, 3, 1,
	12, 0, 5, 9, 10, 2, 3, 7,
	1, 11, 12, 4, 0, 5, 9, 10,
	11, 2, 3, 1, 11, 12, 0, 5,
	9, 10, 2, 3, 7, 1, 5, 11,
	12, 0, 5, 9, 10, 11, 2, 3,
	7, 1, 5, 11, 12, 0, 5, 7,
	5, 1, 5, 11, 7, 5, 9, 10,
	2, 3, 7, 1, 5, 11, 12, 4,
	0, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 0
};

static const char _emoji_presentation_cond_actions[] = {
	15, 0, 15, 11, 15, 15, 15, 15,
	13, 15, 11, 0, 15, 15, 11, 0,
	15, 11, 15, 15, 0, 18, 15, 18,
	5, 15, 0, 5, 15, 9, 15, 15,
	0, 0, 15, 0, 7, 15, 15, 0,
	0, 0, 15, 15, 0, 7, 15, 15,
	0, 0, 15, 0, 15, 15, 0, 7,
	15, 15, 15, 0, 0, 15, 0, 15,
	15, 0, 7, 15, 15, 0, 0, 0,
	15, 0, 7, 15, 15, 0, 0, 15,
	0, 15, 15, 0, 0, 7, 15, 15,
	15, 0, 0, 0, 15, 15, 0, 7,
	15, 15, 0, 0, 15, 0, 5, 15,
	15, 0, 7, 15, 15, 15, 0, 0,
	15, 0, 5, 15, 15, 0, 7, 15,
	9, 0, 5, 15, 15, 9, 15, 15,
	0, 0, 15, 0, 5, 15, 15, 0,
	0, 7, 11, 13, 11, 11, 11, 9,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 9, 9, 7, 0
};

static const char _emoji_presentation_to_state_actions[] = {
	0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _emoji_presentation_from_state_actions[] = {
	0, 0, 0, 0, 0, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _emoji_presentation_eof_cond_spaces[] = {
	-1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, 0
};

static const char _emoji_presentation_eof_cond_key_offs[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _emoji_presentation_eof_cond_key_lens[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _emoji_presentation_eof_cond_keys[] = {
	0
};

static const short _emoji_presentation_eof_trans[] = {
	139, 140, 141, 142, 143, 0, 144, 145,
	146, 147, 148, 149, 150, 151, 152, 153,
	154, 155, 156, 0
};

static const char _emoji_presentation_nfa_targs[] = {
	0, 0
};

static const char _emoji_presentation_nfa_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _emoji_presentation_nfa_push_actions[] = {
	0, 0
};

static const char _emoji_presentation_nfa_pop_trans[] = {
	0, 0
};

static const int emoji_presentation_start = 5;

static const int emoji_presentation_en_text_and_emoji_run = 5;





static gboolean
scan_emoji_presentation (const unsigned char* buffer,
unsigned buffer_size,
unsigned cursor,
unsigned* last,
unsigned* end)
{
	const unsigned char *p = buffer + cursor;
	const unsigned char *pe, *eof, *ts, *te;
	unsigned act;
	int cs;
	pe = eof = buffer + buffer_size;
	
	
	{
		cs = (int)emoji_presentation_start;
		ts = 0;
		te = 0;
		act = 0;
	}
	
	{
		int _cpc;
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;const char * _acts;unsigned int _nacts;	 {
			if ( p == pe )
			goto _test_eof;
			_resume:  {
				_acts = ( _emoji_presentation_actions + (_emoji_presentation_from_state_actions[cs]));
				_nacts = (unsigned int)(*( _acts));
				_acts += 1;
				while ( _nacts > 0 ) {
					switch ( (*( _acts)) ) {
						case 1:  {
							{
								#line 1 "NONE"
								{ts = p;}}
							break; }
					}
					_nacts -= 1;
					_acts += 1;
				}
				
				_keys = ( _emoji_presentation_trans_keys + (_emoji_presentation_key_offsets[cs]));
				_trans = (unsigned int)_emoji_presentation_index_offsets[cs];
				
				_klen = (int)_emoji_presentation_single_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + _klen - 1;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + ((_upper-_lower) >> 1);
						if ( ( (*( p))) < (*( _mid)) )
						_upper = _mid - 1;
						else if ( ( (*( p))) > (*( _mid)) )
						_lower = _mid + 1;
						else {
							_trans += (unsigned int)(_mid - _keys);
							goto _match;
						}
					}
					_keys += _klen;
					_trans += (unsigned int)_klen;
				}
				
				_klen = (int)_emoji_presentation_range_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + (_klen<<1) - 2;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + (((_upper-_lower) >> 1) & ~1);
						if ( ( (*( p))) < (*( _mid)) )
						_upper = _mid - 2;
						else if ( ( (*( p))) > (*( _mid + 1)) )
						_lower = _mid + 2;
						else {
							_trans += (unsigned int)((_mid - _keys)>>1);
							goto _match;
						}
					}
					_trans += (unsigned int)_klen;
				}
				
				_match:  {
					goto _match_cond;
				}
			}
			_match_cond:  {
				cs = (int)_emoji_presentation_cond_targs[_trans];
				
				if ( _emoji_presentation_cond_actions[_trans] == 0 )
				goto _again;
				
				_acts = ( _emoji_presentation_actions + (_emoji_presentation_cond_actions[_trans]));
				_nacts = (unsigned int)(*( _acts));
				_acts += 1;
				while ( _nacts > 0 ) {
					switch ( (*( _acts)) )
					{
						case 2:  {
							{
								#line 1 "NONE"
								{te = p+1;}}
							break; }
						case 3:  {
							{
								#line 71 "emoji_presentation_scanner.rl"
								{act = 1;}}
							break; }
						case 4:  {
							{
								#line 72 "emoji_presentation_scanner.rl"
								{act = 2;}}
							break; }
						case 5:  {
							{
								#line 72 "emoji_presentation_scanner.rl"
								{te = p+1;{
										#line 72 "emoji_presentation_scanner.rl"
										found_text_presentation_sequence }}}
							break; }
						case 6:  {
							{
								#line 71 "emoji_presentation_scanner.rl"
								{te = p;p = p - 1;{
										#line 71 "emoji_presentation_scanner.rl"
										found_emoji_presentation_sequence }}}
							break; }
						case 7:  {
							{
								#line 72 "emoji_presentation_scanner.rl"
								{te = p;p = p - 1;{
										#line 72 "emoji_presentation_scanner.rl"
										found_text_presentation_sequence }}}
							break; }
						case 8:  {
							{
								#line 71 "emoji_presentation_scanner.rl"
								{p = ((te))-1;
									{
										#line 71 "emoji_presentation_scanner.rl"
										found_emoji_presentation_sequence }}}
							break; }
						case 9:  {
							{
								#line 1 "NONE"
								{switch( act ) {
										case 1:  {
											p = ((te))-1;
											{
												#line 71 "emoji_presentation_scanner.rl"
												found_emoji_presentation_sequence } break; }
										case 2:  {
											p = ((te))-1;
											{
												#line 72 "emoji_presentation_scanner.rl"
												found_text_presentation_sequence } break; }
									}}
							}
							break; }
					}
					_nacts -= 1;
					_acts += 1;
				}
				
				
			}
			_again:  {
				_acts = ( _emoji_presentation_actions + (_emoji_presentation_to_state_actions[cs]));
				_nacts = (unsigned int)(*( _acts));
				_acts += 1;
				while ( _nacts > 0 ) {
					switch ( (*( _acts)) ) {
						case 0:  {
							{
								#line 1 "NONE"
								{ts = 0;}}
							break; }
					}
					_nacts -= 1;
					_acts += 1;
				}
				
				p += 1;
				if ( p != pe )
				goto _resume;
			}
			_test_eof:  { {}
				if ( p == eof )
				{
					if ( _emoji_presentation_eof_cond_spaces[cs] != -1 ) {
						_cekeys = ( _emoji_presentation_eof_cond_keys + (_emoji_presentation_eof_cond_key_offs[cs]));
						_klen = (int)_emoji_presentation_eof_cond_key_lens[cs];
						_cpc = 0;
						{
							const char *_lower = _cekeys;
							const char *_upper = _cekeys + _klen - 1;
							const char *_mid;
							while ( 1 ) {
								if ( _upper < _lower )
								break;
								
								_mid = _lower + ((_upper-_lower) >> 1);
								if ( _cpc < (int)(*( _mid)) )
								_upper = _mid - 1;
								else if ( _cpc > (int)(*( _mid)) )
								_lower = _mid + 1;
								else {
									goto _ok;
								}
							}
							cs = -1;
							goto _out;
						}
						_ok: {}
					}
					if ( _emoji_presentation_eof_trans[cs] > 0 ) {
						_trans = (unsigned int)_emoji_presentation_eof_trans[cs] - 1;
						goto _match_cond;
					}
				}
				
			}
			_out:  { {}
			}
		}
	}
	
	return FALSE;
}

