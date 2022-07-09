#line 1 "pango2/emoji_presentation_scanner.rl"
// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#line 6 "pango2/emoji_presentation_scanner.c"
static const signed char _emoji_presentation_actions[] = {
	0, 1, 0, 1, 1, 1, 7, 1,
	8, 1, 9, 1, 10, 1, 11, 1,
	12, 1, 13, 1, 14, 2, 2, 3,
	2, 2, 4, 2, 2, 5, 2, 2,
	6, 0
};

static const signed char _emoji_presentation_key_offsets[] = {
	0, 5, 7, 14, 18, 20, 21, 24,
	29, 30, 34, 36, 0
};

static const unsigned char _emoji_presentation_trans_keys[] = {
	3u, 7u, 13u, 0u, 2u, 14u, 15u, 0u,
	1u, 2u, 3u, 6u, 7u, 13u, 9u, 10u,
	11u, 12u, 10u, 12u, 10u, 4u, 10u, 12u,
	4u, 9u, 10u, 11u, 12u, 6u, 9u, 10u,
	11u, 12u, 8u, 10u, 9u, 10u, 11u, 12u,
	14u, 0u
};

static const signed char _emoji_presentation_single_lengths[] = {
	3, 2, 7, 4, 2, 1, 3, 5,
	1, 4, 2, 5, 0
};

static const signed char _emoji_presentation_range_lengths[] = {
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

static const signed char _emoji_presentation_index_offsets[] = {
	0, 5, 8, 16, 21, 24, 26, 30,
	36, 38, 43, 46, 0
};

static const signed char _emoji_presentation_cond_targs[] = {
	6, 4, 4, 4, 2, 1, 2, 2,
	3, 3, 3, 7, 8, 9, 11, 2,
	2, 0, 2, 5, 2, 0, 5, 2,
	0, 2, 5, 0, 5, 2, 5, 2,
	0, 2, 5, 2, 2, 2, 2, 0,
	2, 10, 2, 2, 0, 2, 2, 0,
	2, 5, 1, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	0
};

static const signed char _emoji_presentation_cond_actions[] = {
	27, 27, 27, 27, 19, 0, 7, 17,
	30, 21, 27, 27, 0, 30, 27, 9,
	7, 0, 5, 24, 19, 0, 27, 13,
	0, 19, 27, 0, 27, 13, 27, 7,
	0, 5, 24, 13, 7, 15, 7, 0,
	5, 24, 15, 7, 0, 11, 7, 0,
	5, 24, 0, 13, 19, 17, 0, 19,
	13, 19, 13, 13, 15, 15, 11, 13,
	0
};

static const signed char _emoji_presentation_to_state_actions[] = {
	0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

static const signed char _emoji_presentation_from_state_actions[] = {
	0, 0, 3, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

static const signed char _emoji_presentation_eof_trans[] = {
	53, 54, 55, 56, 57, 58, 59, 60,
	61, 62, 63, 64, 0
};

static const int emoji_presentation_start = 2;

static const int emoji_presentation_en_text_and_emoji_run = 2;


#line 9 "pango2/emoji_presentation_scanner.rl"



#line 81 "pango2/emoji_presentation_scanner.rl"


static emoji_text_iter_t
scan_emoji_presentation (emoji_text_iter_t p,
const emoji_text_iter_t pe,
EmojiPresentation *state,
gboolean *explicit)
{
	emoji_text_iter_t ts, te;
	const emoji_text_iter_t eof = pe;
	
	unsigned act;
	int cs;
	

#line 104 "pango2/emoji_presentation_scanner.c"
	{
		cs = (int)emoji_presentation_start;
		ts = 0;
		te = 0;
		act = 0;
	}

#line 110 "pango2/emoji_presentation_scanner.c"
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
					
#line 129 "pango2/emoji_presentation_scanner.c"

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
						
#line 210 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 3:  {
							{
#line 75 "pango2/emoji_presentation_scanner.rl"
							{act = 2;}}
						
#line 218 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 4:  {
							{
#line 76 "pango2/emoji_presentation_scanner.rl"
							{act = 3;}}
						
#line 226 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 5:  {
							{
#line 77 "pango2/emoji_presentation_scanner.rl"
							{act = 4;}}
						
#line 234 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 6:  {
							{
#line 78 "pango2/emoji_presentation_scanner.rl"
							{act = 5;}}
						
#line 242 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 7:  {
							{
#line 74 "pango2/emoji_presentation_scanner.rl"
							{te = p+1;{
#line 74 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_TEXT; *explicit = TRUE; return te; }
							}}
						
#line 253 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 8:  {
							{
#line 77 "pango2/emoji_presentation_scanner.rl"
							{te = p+1;{
#line 77 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_EMOJI; *explicit = FALSE; return te; }
							}}
						
#line 264 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 9:  {
							{
#line 78 "pango2/emoji_presentation_scanner.rl"
							{te = p+1;{
#line 78 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_NONE; *explicit = TRUE; return te; }
							}}
						
#line 275 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 10:  {
							{
#line 76 "pango2/emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 76 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_EMOJI; *explicit = TRUE; return te; }
							}}
						
#line 286 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 11:  {
							{
#line 77 "pango2/emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 77 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_EMOJI; *explicit = FALSE; return te; }
							}}
						
#line 297 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 12:  {
							{
#line 78 "pango2/emoji_presentation_scanner.rl"
							{te = p;p = p - 1;{
#line 78 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_NONE; *explicit = TRUE; return te; }
							}}
						
#line 308 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 13:  {
							{
#line 77 "pango2/emoji_presentation_scanner.rl"
							{p = ((te))-1;
								{
#line 77 "pango2/emoji_presentation_scanner.rl"
									*state = EMOJI_PRESENTATION_EMOJI; *explicit = FALSE; return te; }
							}}
						
#line 320 "pango2/emoji_presentation_scanner.c"

						break; 
					}
					case 14:  {
							{
#line 1 "NONE"
							{switch( act ) {
									case 2:  {
										p = ((te))-1;
										{
#line 75 "pango2/emoji_presentation_scanner.rl"
											*state = EMOJI_PRESENTATION_TEXT; *explicit = FALSE; return te; }
										break; 
									}
									case 3:  {
										p = ((te))-1;
										{
#line 76 "pango2/emoji_presentation_scanner.rl"
											*state = EMOJI_PRESENTATION_EMOJI; *explicit = TRUE; return te; }
										break; 
									}
									case 4:  {
										p = ((te))-1;
										{
#line 77 "pango2/emoji_presentation_scanner.rl"
											*state = EMOJI_PRESENTATION_EMOJI; *explicit = FALSE; return te; }
										break; 
									}
									case 5:  {
										p = ((te))-1;
										{
#line 78 "pango2/emoji_presentation_scanner.rl"
											*state = EMOJI_PRESENTATION_NONE; *explicit = TRUE; return te; }
										break; 
									}
								}}
						}
						
#line 358 "pango2/emoji_presentation_scanner.c"

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
						
#line 383 "pango2/emoji_presentation_scanner.c"

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
	
#line 98 "pango2/emoji_presentation_scanner.rl"

	
	/* Should not be reached. */
	*state = EMOJI_PRESENTATION_NONE;
	*explicit = FALSE;
	return pe;
}
