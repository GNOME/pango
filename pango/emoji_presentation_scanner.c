#line 1 "emoji_presentation_scanner.rl"
/* Copyright 2019 Google LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     https://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdbool.h>

#ifndef EMOJI_LINKAGE
#define EMOJI_LINKAGE static
#endif


#line 23 "emoji_presentation_scanner.c"
static const signed char _emoji_presentation_actions[] = {
	0, 1, 0, 1, 1, 1, 6, 1,
	7, 1, 8, 1, 9, 1, 10, 1,
	11, 1, 12, 1, 13, 1, 14, 1,
	15, 2, 2, 3, 2, 2, 4, 2,
	2, 5, 0
};

static const signed char _emoji_presentation_key_offsets[] = {
	0, 5, 7, 14, 18, 20, 21, 24,
	29, 30, 34, 35, 37, 0
};

static const unsigned char _emoji_presentation_trans_keys[] = {
	3u, 7u, 13u, 0u, 2u, 14u, 15u, 2u,
	3u, 6u, 7u, 13u, 0u, 1u, 9u, 10u,
	11u, 12u, 10u, 12u, 10u, 4u, 10u, 12u,
	4u, 9u, 10u, 11u, 12u, 6u, 9u, 10u,
	11u, 12u, 8u, 8u, 10u, 9u, 10u, 11u,
	12u, 14u, 0u
};

static const signed char _emoji_presentation_single_lengths[] = {
	3, 2, 5, 4, 2, 1, 3, 5,
	1, 4, 1, 2, 5, 0
};

static const signed char _emoji_presentation_range_lengths[] = {
	1, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};

static const signed char _emoji_presentation_index_offsets[] = {
	0, 5, 8, 15, 20, 23, 25, 29,
	35, 37, 42, 44, 47, 0
};

static const signed char _emoji_presentation_cond_targs[] = {
	6, 4, 4, 4, 2, 1, 2, 2,
	3, 7, 8, 9, 12, 3, 2, 2,
	0, 2, 5, 2, 0, 5, 2, 0,
	2, 5, 0, 5, 2, 5, 2, 0,
	2, 5, 2, 2, 2, 2, 0, 10,
	11, 2, 2, 2, 2, 0, 2, 2,
	0, 2, 5, 1, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 0
};

static const signed char _emoji_presentation_cond_actions[] = {
	28, 28, 28, 28, 23, 0, 9, 21,
	28, 28, 0, 31, 28, 31, 11, 9,
	0, 5, 25, 23, 0, 28, 17, 0,
	23, 28, 0, 28, 17, 28, 9, 0,
	5, 25, 17, 9, 19, 9, 0, 0,
	25, 19, 5, 13, 7, 0, 15, 9,
	0, 5, 25, 0, 17, 23, 21, 0,
	23, 17, 23, 17, 17, 19, 19, 13,
	15, 17, 0
};

static const signed char _emoji_presentation_to_state_actions[] = {
	0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};

static const signed char _emoji_presentation_from_state_actions[] = {
	0, 0, 3, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};

static const signed char _emoji_presentation_eof_trans[] = {
	54, 55, 56, 57, 58, 59, 60, 61,
	62, 63, 64, 65, 66, 0
};

static const int emoji_presentation_start = 2;

static const int emoji_presentation_en_text_and_emoji_run = 2;


#line 26 "emoji_presentation_scanner.rl"



#line 100 "emoji_presentation_scanner.rl"


EMOJI_LINKAGE emoji_text_iter_t
scan_emoji_presentation (emoji_text_iter_t p,
const emoji_text_iter_t pe,
bool* is_emoji,
bool* has_vs)
{
	emoji_text_iter_t ts;
	emoji_text_iter_t te;
	const emoji_text_iter_t eof = pe;
	
	(void)ts;
	
	unsigned act;
	int cs;
	

#line 124 "emoji_presentation_scanner.c"
	{
		cs = (int)emoji_presentation_start;
		ts = 0;
		te = 0;
		act = 0;
	}

#line 130 "emoji_presentation_scanner.c"
	{
		int _klen;
		unsigned int _trans = 0;
		const unsigned char * _keys;
		const signed char * _acts;
		unsigned int _nacts;
		_resume: {}
		if ( p == pe && p != eof )
			goto _out;
		_acts = ( _emoji_presentation_actions + (_emoji_presentation_from_state_actions[cs]));
		_nacts = (unsigned int)(*( _acts));
		_acts += 1;
		while ( _nacts > 0 ) {
			switch ( (*( _acts)) ) {
				case 1:  {
						{
#line 1 "NONE"
						{ts = p;}}
					
#line 149 "emoji_presentation_scanner.c"

					break; 
				}
			}
			_nacts -= 1;
			_acts += 1;
		}
		
		if ( p == eof ) {
			if ( _emoji_presentation_eof_trans[cs] > 0 ) {
				_trans = (unsigned int)_emoji_presentation_eof_trans[cs] - 1;
			}
		}
		else {
			_keys = ( _emoji_presentation_trans_keys + (_emoji_presentation_key_offsets[cs]));
			_trans = (unsigned int)_emoji_presentation_index_offsets[cs];
			
			_klen = (int)_emoji_presentation_single_lengths[cs];
			if ( _klen > 0 ) {
				const unsigned char *_lower = _keys;
				const unsigned char *_upper = _keys + _klen - 1;
				const unsigned char *_mid;
				while ( 1 ) {
					if ( _upper < _lower ) {
						_keys += _klen;
						_trans += (unsigned int)_klen;
						break;
					}
					
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
			}
			
			_klen = (int)_emoji_presentation_range_lengths[cs];
			if ( _klen > 0 ) {
				const unsigned char *_lower = _keys;
				const unsigned char *_upper = _keys + (_klen<<1) - 2;
				const unsigned char *_mid;
				while ( 1 ) {
					if ( _upper < _lower ) {
						_trans += (unsigned int)_klen;
						break;
					}
					
					_mid = _lower + (((_upper-_lower) >> 1) & ~1);
					if ( ( (*( p))) < (*( _mid)) )
						_upper = _mid - 2;
					else if ( ( (*( p))) > (*( _mid + 1)) )
						_lower = _mid + 2;
					else {
						_trans += (unsigned int)((_mid - _keys)>>1);
						break;
					}
				}
			}
			
			_match: {}
		}
		cs = (int)_emoji_presentation_cond_targs[_trans];
		
		if ( _emoji_presentation_cond_actions[_trans] != 0 ) {
			
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
						
#line 230 "emoji_presentation_scanner.c"

						break; 
					}
					case 3:  {
							{
#line 95 "emoji_presentation_scanner.rl"
							{act = 2;}}
						
#line 238 "emoji_presentation_scanner.c"

						break; 
					}
					case 4:  {
							{
#line 96 "emoji_presentation_scanner.rl"
							{act = 3;}}
						
#line 246 "emoji_presentation_scanner.c"

						break; 
					}
					case 5:  {
							{
#line 97 "emoji_presentation_scanner.rl"
							{act = 4;}}
						
#line 254 "emoji_presentation_scanner.c"

						break; 
					}
					case 6:  {
							{
#line 94 "emoji_presentation_scanner.rl"
							{te = p+1;{
#line 94 "emoji_presentation_scanner.rl"
									*is_emoji = false; *has_vs = true; return te; }
							}}
						
#line 265 "emoji_presentation_scanner.c"

						break; 
					}
					case 7:  {
							{
#line 95 "emoji_presentation_scanner.rl"
							{te = p+1;{
#line 95 "emoji_presentation_scanner.rl"
									*is_emoji = true; *has_vs = true; return te; }
							}}
						
#line 276 "emoji_presentation_scanner.c"

						break; 
					}
					case 8:  {
							{
#line 96 "emoji_presentation_scanner.rl"
							{te = p+1;{
#line 96 "emoji_presentation_scanner.rl"
									*is_emoji = true; *has_vs = false; return te; }
							}}
						
#line 287 "emoji_presentation_scanner.c"

						break; 
					}
					case 9:  {
							{
#line 97 "emoji_presentation_scanner.rl"
							{te = p+1;{
#line 97 "emoji_presentation_scanner.rl"
									*is_emoji = false; *has_vs = false; return te; }
							}}
						
#line 298 "emoji_presentation_scanner.c"

						break; 
					}
					case 10:  {
							{
#line 94 "emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 94 "emoji_presentation_scanner.rl"
									*is_emoji = false; *has_vs = true; return te; }
							}}
						
#line 309 "emoji_presentation_scanner.c"

						break; 
					}
					case 11:  {
							{
#line 95 "emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 95 "emoji_presentation_scanner.rl"
									*is_emoji = true; *has_vs = true; return te; }
							}}
						
#line 320 "emoji_presentation_scanner.c"

						break; 
					}
					case 12:  {
							{
#line 96 "emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 96 "emoji_presentation_scanner.rl"
									*is_emoji = true; *has_vs = false; return te; }
							}}
						
#line 331 "emoji_presentation_scanner.c"

						break; 
					}
					case 13:  {
							{
#line 97 "emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 97 "emoji_presentation_scanner.rl"
									*is_emoji = false; *has_vs = false; return te; }
							}}
						
#line 342 "emoji_presentation_scanner.c"

						break; 
					}
					case 14:  {
							{
#line 96 "emoji_presentation_scanner.rl"
							{p = ((te))-1;
								{
#line 96 "emoji_presentation_scanner.rl"
									*is_emoji = true; *has_vs = false; return te; }
							}}
						
#line 354 "emoji_presentation_scanner.c"

						break; 
					}
					case 15:  {
							{
#line 1 "NONE"
							{switch( act ) {
									case 2:  {
										p = ((te))-1;
										{
#line 95 "emoji_presentation_scanner.rl"
											*is_emoji = true; *has_vs = true; return te; }
										break; 
									}
									case 3:  {
										p = ((te))-1;
										{
#line 96 "emoji_presentation_scanner.rl"
											*is_emoji = true; *has_vs = false; return te; }
										break; 
									}
									case 4:  {
										p = ((te))-1;
										{
#line 97 "emoji_presentation_scanner.rl"
											*is_emoji = false; *has_vs = false; return te; }
										break; 
									}
								}}
						}
						
#line 385 "emoji_presentation_scanner.c"

						break; 
					}
				}
				_nacts -= 1;
				_acts += 1;
			}
			
		}
		
		if ( p == eof ) {
			if ( cs >= 2 )
				goto _out;
		}
		else {
			_acts = ( _emoji_presentation_actions + (_emoji_presentation_to_state_actions[cs]));
			_nacts = (unsigned int)(*( _acts));
			_acts += 1;
			while ( _nacts > 0 ) {
				switch ( (*( _acts)) ) {
					case 0:  {
							{
#line 1 "NONE"
							{ts = 0;}}
						
#line 410 "emoji_presentation_scanner.c"

						break; 
					}
				}
				_nacts -= 1;
				_acts += 1;
			}
			
			p += 1;
			goto _resume;
		}
		_out: {}
	}
	
#line 120 "emoji_presentation_scanner.rl"

	
	/* Should not be reached. */
	*is_emoji = false;
	*has_vs = false;
	return p;
}

