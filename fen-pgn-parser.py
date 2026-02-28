
from datetime import datetime
import ctypes
import os

engine_path = os.path.abspath("engine.so")
engine = ctypes.CDLL(engine_path)

engine.is_draw.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_draw.restype = ctypes.c_int

engine.is_mate.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_mate.restype = ctypes.c_int

engine.is_legal_move.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_legal_move.restype = ctypes.c_int

engine.is_check.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_check.restype = ctypes.c_int


def print_board(board):
	for y in range(8):
		for x in range(8):
			print(board[y*8 + x], end='')
		print()

def make_board(fen):
	fen_board = fen.split(' ')[0]
	fen_rows = fen_board.split('/')
	
	board = []
	rows = []
	
	for fen_row in fen_rows:
		row = []
		for square in fen_row:
			if square.isdigit():
				for i in range(int(square)):
					row.append(0)
			else:
				row.append(square)
		rows.append(row)
	
	for row in rows:
		for square in row:
			board.append(square)
				
	return board

def get_changed_squares(board1, board2):# [(sq1, sq2, i), ...]
	
	changed = []
	for i in range(64):
		if board1[i] != board2[i]:
			changed.append((board1[i], board2[i], i))
	return changed
	
def get_move_type(changed):
	if len(changed) == 4:
		return 'castle'
	elif len(changed) == 3:
		return 'en passant'
	elif len(changed) == 2:
		if (changed[0][0] in ['p', 'P'] and changed[0][1] == 0 and changed[1][2]//8 in [0,7]) or (changed[1][0] in ['p', 'P'] and changed[1][1] == 0 and changed[0][2]//8 in [0,7]):
			for change in changed:
				if change[2] in [0,7] and change[0] != 0:
					return 'promotion capture'
			return 'promotion'
		elif changed[0][0] != 0 and changed[1][0] != 0:
			return 'normal capture'
		else:
			return 'normal move'
	else:
		return 'invalid move'

def get_move_info(changed):# (piece, start_i, end_i, is_capture, is_ep, promoted_to) / (king, castle)
	
	move_type = get_move_type(changed)
	
	promoted_to = None
	piece = ''
	start_i, end_i = -1, -1
	is_ep = False
	is_capture = False
	
	if move_type == 'castle':
		for change in changed:
			if change[0] in ['r', 'R']:
				if change[2] in [0, 56]:
					return ('K' if (change[0].isupper) else 'k', 'O-O-O')
				elif change[2] in [7, 63]:
					return ('K' if (change[0].isupper) else 'k', 'O-O')
					
	elif move_type == 'en passant':
		is_ep = True
		is_capture = True
		for change in changed:
			if change[0] == 0:
				piece = change[1]
				end_i = change[2]
		for change in changed:
			if change[0] == piece:
				start_i = change[2]
				
	elif move_type in ['normal capture', 'normal move']:
		for change in changed:
			if change[1] == 0:
				piece = change[0]
				start_i = change[2]
			else:
				end_i = change[2]
	elif move_type in ['promotion capture', 'promotion']:
		for change in changed:
			if change[1] == 0:
				piece = change[0]
				start_i = change[2]
			else:
				promoted_to = change[1]
				end_i = change[2]
	
	if move_type in ['normal capture', 'promotion capture']:
		is_capture = True
				
	return (piece, start_i, end_i, is_capture, is_ep, promoted_to)

def get_square_by_index(index):
	
	x_to_letter = {0:'a', 1:'b', 2:'c', 3:'d', 4:'e', 5:'f', 6:'g', 7:'h'}
	
	y = index // 8
	x = index % 8
	
	x_coord = x_to_letter[x]
	y_coord = 8 - y
	
	return x_coord + str(y_coord)

def get_ambiguity_resolver(board1, board0, move_info):
	
	board1_string = ''.join([str(square) for square in board1])
	board = bytearray(board1_string, 'utf_8')
	board = (ctypes.c_char * len(board))(*board)
	
	board0_string = ''.join([str(square) for square in board0])
	previous_board = bytearray(board0_string, 'utf_8')
	previous_board = (ctypes.c_char * len(previous_board))(*previous_board)
	
	ambiguous = False
	non_ambiguous = ['x', 'y']
	
	piece = move_info[0]
	start_index = move_info[1]
	end_index = move_info[2]
	is_capture = move_info[3]
	
	piece_x, piece_y = start_index%8, 7-start_index//8
	end_x, end_y = end_index%8, 7-end_index//8
	
	for i in range(64):
		square = board1[i]
		if (square == piece):
			start_x, start_y = i%8, 7-i//8
			vec_x, vec_y = end_x-start_x, end_y-start_y
			
			if engine.is_legal_move(board, ord(piece), vec_x, vec_y, end_x, end_y, previous_board) and (piece_x, piece_y) != (start_x, start_y):
				ambiguous = True
				if piece_x == start_x and 'x' in non_ambiguous:
					non_ambiguous.remove('x')
				elif piece_y == start_y and 'y' in non_ambiguous:
					non_ambiguous.remove('y')
	
	if ambiguous:
		if 'x' in non_ambiguous:
			return 'x'
		elif 'y' in non_ambiguous:
			return 'y'
		else:
			return 'x and y'
	else:
		return '-'

def format_to_pgn(board, next_board, previous_board, move_info, move_number):
	
	turn = 'w' if (move_info[0].isupper()) else 'b'
	
	if move_info[1] in ['O-O', 'O-O-O']:
		move = move_number + '.' +  ' ' + move_info[1]
	else:
		promoted_to = move_info[-1]
		move_info = move_info[:-1]
		piece = move_info[0].upper()
		start_square = get_square_by_index(move_info[1])
		end_square = get_square_by_index(move_info[2])
		is_capture = move_info[3]
		
		resolver = get_ambiguity_resolver(board, previous_board, move_info)
		
		if resolver != '-':
			if piece in ['p', 'P']:
				if resolver == 'x':
					if is_capture:
						move = move_number + '.' + ' ' + start_square[0] + 'x' + end_square
					else:
						move = move_number + '.' + ' ' + start_square[0] + end_square
				elif resolver == 'y':
					if is_capture:
						move = move_number + '.' + ' ' + start_square[1] + 'x' + end_square
					else:
						move = move_number + '.' + ' ' + start_square[1] + end_square
				else:
					if is_capture:
						move = move_number + '.' + ' ' + start_square + 'x' + end_square
					else:
						move = move_number + '.' + ' ' + start_square + end_square
			elif resolver == 'x':
				if is_capture:
					move = move_number + '.' + ' ' + piece + start_square[0] + 'x' + end_square
				else:
					move = move_number + '.' + ' ' + piece + start_square[0] + end_square
			elif resolver == 'y':
				if is_capture:
					move = move_number + '.' + ' ' + piece + start_square[1] + 'x' + end_square
				else:
					move = move_number + '.' + ' ' + piece + start_square[1] + end_square
			else:
				if is_capture:
					move = move_number + '.' + ' ' + piece + start_square + 'x' + end_square
				else:
					move = move_number + '.' + ' ' + piece + start_square + end_square
		else:
			if piece in ['p', 'P']:
				if is_capture:
					move = move_number + '.' + ' ' + start_square[0] + 'x' + end_square
				else:
					move = move_number + '.' + ' ' + end_square
			else:
				if is_capture:
					move = move_number + '.' + ' ' + piece + 'x' + end_square
				else:
					move = move_number + '.' + ' ' + piece + end_square
		
		if promoted_to != None:
			move = move + '=' + promoted_to
	
	board0_string = ''.join([str(square) for square in previous_board])
	board0 = bytearray(board0_string, 'utf_8')
	board0 = (ctypes.c_char * len(board0))(*board0)
	
	board1_string = ''.join([str(square) for square in board])
	board1 = bytearray(board1_string, 'utf_8')
	board1 = (ctypes.c_char * len(board1))(*board1)
	
	board2_string = ''.join([str(square) for square in next_board])
	board2 = bytearray(board2_string, 'utf_8')
	board2 = (ctypes.c_char * len(board2))(*board2)
	
	if engine.is_check(board2, ord('b' if (turn == 'w') else 'w')):
		move += '+'
	
	if engine.is_mate(board2, board1, ord('b' if (turn == 'w') else 'w')):
		
		if turn == 'w':
			postfix = ' 1-0'
		else:
			postfix = ' 0-1'
		move = move[:-1] + '#' + postfix
	
	if engine.is_draw(board2, board1, ord('b' if (turn == 'w') else 'w')):
		move = move + ' 1/2-1/2'
	
	return move

def parse_pgn_notation(moves):
	
	modified_moves = []
	number = 0
	
	for move in moves:
		prev_number = number
		number = move.split(' ')[0]
		move_note = move.split(' ')[1]
		
		if len(move.split(' ')) >= 3:
			postfix = move.split(' ')[2]
		else:
			postfix = ''
		
		if prev_number == number:
			modified_moves[-1] += ' ' + move_note + ' ' + postfix
		else:
			modified_moves.append(move)
	
	return modified_moves

def parse_tags(pgn_moves):
	
	last_move = pgn_moves[-1]
	result = last_move.split(' ')[-1]
	if result not in ['1/2-1/2', '1-0', '0-1']:
		result = '*'
		pgn_moves[-1] += ' *'
	
	date = datetime.now().strftime('%Y.%m.%d')
	
	black_player = input('Who played as black:\t')
	white_player = input('Who played as white:\t')
	event = input('Event the game was played in:\t')
	site = input('Site the game was played in:\t')
	round_of_the_event = input('Round of the event:\t')
	
	black_player = black_player if (black_player != '') else '?'
	white_player = white_player if (white_player != '') else '?'
	event = event if (event != '') else '?'
	site = site if (site != '') else '?'
	round_of_the_event = round_of_the_event if (round_of_the_event != '') else '?'
	
	pgn_tags = f'[Event "{event}"]\n[Site "{site}"]\n[Date "{date}"]\n[Round "{round_of_the_event}"]\n[White "{white_player}"]\n[Black "{black_player}"]\n[Result "{result}"]\n'
	return pgn_tags


fens = []
file_path = ''

while file_path == '':
	try:
		filename = input("filename of the game in FEN-notation:\n")
		file_path = 'games/' + filename
		with open(file_path, 'r') as fen_file:
			fens = fen_file.readlines()
	except FileNotFoundError:
		file_path = ''
	

moves = []

for i in range(len(fens)):
	fen = fens[i]
	if fen[-1] == '\n':
		fens[i] = fen.rstrip()

for i in range(len(fens)):
	
	fen0 = fens[i-1] if (i >= 1) else fens[i]
	fen1 = fens[i]
	fen2 = fens[i+1] if (i < len(fens) - 1) else None
	
	move_number = fen1.split(' ')[-1]
	
	if fen2:
		previous_board = make_board(fen0)
		board1 = make_board(fen1)
		board2 = make_board(fen2)
		
		changed = get_changed_squares(board1, board2)
		move_info = get_move_info(changed)
		
		move = format_to_pgn(board1, board2, previous_board, move_info, move_number)
		moves.append(move)



pgn_moves = parse_pgn_notation(moves)

if len(pgn_moves) != 0:
	pgn_tags = parse_tags(pgn_moves)
	full_pgn = '\n'.join([pgn_tags, '\n'.join(pgn_moves)])
else:
	full_pgn = 'Fen notation is invalid.'

print('\n\nHere is a prewiew of the pgn-file:\n\n')
print(full_pgn)
print()

success = False
while (not success):
	try:
		pgn_filename = input('filename (Note that if a file with the same name exists, it will be overwritten):\t')
		pgn_path = 'games/' + pgn_filename
		with open(pgn_path, 'w') as pgn_file:
			pgn_file.write(full_pgn)
			success = True
	except IsADirectoryError:
		pass







