
import os
import ctypes
import sys
import pygame
pygame.init()

engine_path = os.path.abspath("engine.so")
engine = ctypes.CDLL(engine_path)

engine.is_legal_move.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_legal_move.restype = ctypes.c_int

engine.is_draw.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_draw.restype = ctypes.c_int

engine.is_mate.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_char), ctypes.c_char]
engine.is_mate.restype = ctypes.c_int

engine.made_move.argtypes = [ctypes.POINTER(ctypes.c_char)]


class MainScreen:
	def __init__(self, height, width):
		self.height = height
		self.width = width
		
		self.scr = pygame.display.set_mode((height, width), pygame.RESIZABLE)
		pygame.display.set_caption("The Engine 1.0")
		self.font = pygame.font.SysFont("Arial", 50)
		
	def get_squares(self, margin, treshold):
		
		h = (self.height - margin*2 if (self.height > treshold) else treshold - margin*2)/8
		w = h
		
		squares = []
		dark = (90,40,0)
		light = (255,205,150)
		
		for y in range(8):
			for x in range(8):
				square = pygame.Rect(x*w + (self.width - w*8)/2, y*h + margin, w, h)
				squares.append((square, dark if ((x+y)%2) else light))
		return squares

def import_images():
	P0 = pygame.image.load("images/p0.png").convert_alpha()
	P1 = pygame.image.load("images/p1.png").convert_alpha()
	N0 = pygame.image.load("images/N0.png").convert_alpha()
	N1 = pygame.image.load("images/N1.png").convert_alpha()
	B0 = pygame.image.load("images/B0.png").convert_alpha()
	B1 = pygame.image.load("images/B1.png").convert_alpha()
	R0 = pygame.image.load("images/R0.png").convert_alpha()
	R1 = pygame.image.load("images/R1.png").convert_alpha()
	Q0 = pygame.image.load("images/Q0.png").convert_alpha()
	Q1 = pygame.image.load("images/Q1.png").convert_alpha()
	K0 = pygame.image.load("images/K0.png").convert_alpha()
	K1 = pygame.image.load("images/K1.png").convert_alpha()
	
	images_by_name = {"P": P0,
						"p": P1,
						"N": N0,
						"n": N1,
						"B": B0,
						"b": B1,
						"R": R0,
						"r": R1,
						"Q": Q0,
						"q": Q1,
						"K": K0,
						"k": K1
						}
	return images_by_name
		
def draw_board(margin, treshold):
	scr = window.scr
	squaresWithInfo = window.get_squares(margin, treshold)
	for squareInfo in squaresWithInfo:
		pygame.draw.rect(scr, squareInfo[1], squareInfo[0])
		
def draw_pieces(fen, margin, treshold):
	scr = window.scr
	board = make_board(fen)
	shrinking_factor = 10
	
	for y in range(8):
		for x in range(8):
			square = board[y*8 + x]
			
			if square == 0:
				continue
			else:
				if window.height > treshold:
					board_h = window.height - margin*2
				else:
					board_h = treshold - margin*2
				board_w = board_h
				
				square_h = board_h / 8
				square_w = board_h / 8
				
				img_size = IMAGES_BY_SYMBOL[square].get_size()
				aspect_ratio = img_size[1] / img_size[0]
				
				new_w = square_w / aspect_ratio
				new_h = square_h - shrinking_factor
				
				og_img = IMAGES_BY_SYMBOL[square]
				img = pygame.transform.scale(og_img, (new_w, new_h))
				
				pic_x = (window.width - board_w)/2 + x*square_w + (square_w - new_w)/2
				pic_y = margin + y*square_h + shrinking_factor/2
				scr.blit(img, (pic_x, pic_y))
				
def get_square_index(mouse_pos, margin, treshold):
	squaresWithInfo = window.get_squares(margin, treshold)
	
	for squareInfo in squaresWithInfo:
		square = squareInfo[0]
		if square.collidepoint(mouse_pos):
			return squaresWithInfo.index(squareInfo)

def get_castling_vars(fen, piece, start_index, end_index):
	
	castlings = list(fen.split(" ")[2])
	illegal_castlings = []
	
	if piece == 'r' or end_index in [0, 7]: # If end_index is where the rook was, the rook got taken.
		if 0 in [start_index, end_index]:
			illegal_castlings.append('q')
		elif 7 in [start_index, end_index]:
			illegal_castlings.append('k')
	if piece == 'R' or end_index in [56, 63]:
		if 56 in [start_index, end_index]:
			illegal_castlings.append('Q')
		elif 63 in [start_index, end_index]:
			illegal_castlings.append('K')
	if piece == 'k':
		illegal_castlings.append('k')
		illegal_castlings.append('q')
	if piece == 'K':
		illegal_castlings.append('K')
		illegal_castlings.append('Q')
			
	for illegal_castling in illegal_castlings:
		if illegal_castling in castlings:
			castlings.remove(illegal_castling)
	
	if len(castlings) == 0:
		castlings = '-'
	else:
		castlings = "".join(castlings)
	
	return castlings

def make_board(fen):
	board_string = fen.split(' ')[0]
	rows_as_strings = board_string.split('/')
	
	board = []
	rows = []
	
	for string_row in rows_as_strings:
		row = []
		for square in string_row:
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

def handle_pawn_move(board, piece, start_index, end_index):
	
	# When a pawn takes another piece via a normal move there is no need to remove the taken piece in this function as it will be removed later.
	if piece == 'p' or piece == 'P':
		if start_index - end_index not in [16, 8, -8, -16]:
			if board[end_index] == 0:
				if piece == 'P':
					board[end_index + 8] = 0
				else:
					board[end_index - 8] = 0
			return True
		else:
			return True
	
	return False

def handle_castling(board, piece, start_index, end_index):
	
	if piece in ['k', 'K'] and end_index - start_index in [2, -2]:
		
		if start_index == 4 and end_index == 2 and piece == 'k':
			board[0] = 0
			board[3] = 'r'
		elif start_index == 4 and end_index == 6 and piece == 'k':
			board[7] = 0
			board[5] = 'r'
			
		if start_index == 60 and end_index == 62 and piece == 'K':
			board[63] = 0
			board[61] = 'R'
		elif start_index == 60 and end_index == 58 and piece == 'K':
			board[56] = 0
			board[59] = 'R'

def parse_fen_board(board):
	
	string_board = []
	
	rows = []
	for y in range(8):
		row = []
		for x in range(8):
			row.append(board[y*8 + x])
		rows.append(row)
		
	for row in rows:
		zeros = 0
		for i in range(len(row)):
			square = row[i]
			if square == 0:
				zeros += 1
				if i == 7:
					string_board.append(str(zeros))
			else:
				if zeros != 0:
					string_board.append(str(zeros))
					zeros = 0
				string_board.append(square)
		string_board.append('/')
	del string_board[-1]
	
	return "".join(string_board)

def get_ep_square(piece, start_index, end_index):
	
	x_to_letter = {0:'a', 1:'b', 2:'c', 3:'d', 4:'e', 5:'f', 6:'g', 7:'h'}
	
	ep_index = -1
	if piece == 'P' and start_index - end_index == 16: # Here start_index has higher numeric value than end_index because of how the board is set up.
		ep_index = end_index + 8
	elif piece == 'p' and end_index - start_index == 16:
		ep_index = end_index - 8
	else:
		return '-'
		
	x = x_to_letter[ep_index%8]
	y = str(8 - ep_index//8)
	
	return x + y

def get_new_fen(fen, piece_index, end_index):
	
	piece_taken = False
	fen_as_list = fen.split(' ')
	board = make_board(fen)
	piece = board[piece_index]
	
	if isinstance(piece, int):
		return fen
	
	piece_taken = handle_pawn_move(board, piece, piece_index, end_index)
	handle_castling(board, piece, piece_index, end_index)
	
	if board[end_index] != 0:
		piece_taken = True
	
	board[piece_index] = 0
	board[end_index] = piece
	
	if (piece == 'P' and end_index//8 == 0) or (piece == 'p' and end_index//8 == 7):
		board[end_index] = get_promoted_piece('w' if piece == 'P' else 'b')
		
		
	fen_board = parse_fen_board(board)
	new_turn = 'w' if (fen_as_list[1] == 'b') else 'b'
	castlings = get_castling_vars(fen, piece, piece_index, end_index)
	ep_square = get_ep_square(piece, piece_index, end_index)
	half_moves = '0' if (piece_taken or piece in ['p', 'P']) else str(int(fen_as_list[4]) + 1)
	
	moves = int(fen_as_list[-1])
	if new_turn == 'w':
		moves += 1
	moves = str(moves)
	
	new_fen = ''.join([fen_board, ' ', new_turn, ' ', castlings, ' ', ep_square, ' ', half_moves, ' ', moves])
	
	return new_fen

def get_color_by_index(index, fen):
	
	if index == None:
		return "-"
		
	fen_board = fen.split(" ")[0]
	board = []
	
	for letter in fen_board:
		if letter.isdigit():
			for i in range(int(letter)):
				board.append("0")
		elif letter == "/":
			continue
		else:
			board.append(letter)
	
	piece = board[index]
	return "w" if piece.isupper() else "b" if piece.islower() else "-"

def get_promoted_piece(turn):
	
	if turn == 'w':
		n = pygame.image.load("images/N0.png").convert_alpha()
		b = pygame.image.load("images/B0.png").convert_alpha()
		r = pygame.image.load("images/R0.png").convert_alpha()
		q = pygame.image.load("images/Q0.png").convert_alpha()
	else:
		n = pygame.image.load("images/N1.png").convert_alpha()
		b = pygame.image.load("images/B1.png").convert_alpha()
		r = pygame.image.load("images/R1.png").convert_alpha()
		q = pygame.image.load("images/Q1.png").convert_alpha()
	
	pieces = [n, b, r, q]
	piece_locations = []
	
	window_w, window_h = window.scr.get_size()
	
	dark_overlay = pygame.Surface((window_w, window_h))
	dark_overlay.fill((0,0,0))
	dark_overlay.set_alpha(100)
	window.scr.blit(dark_overlay, (0,0))
	
	for i in range(len(pieces)):
		
		piece = pieces[i]
		piece_w, piece_h = piece.get_size()
		
		aspect_ratio = piece_w / piece_h
		pic_h = window_h // 4
		pic_w = int(pic_h * aspect_ratio)
		
		piece = pygame.transform.scale(piece, (pic_w, pic_h))
		piece_x = i * (window_w // 4) + (window_w // 8) - (pic_w // 2)
		piece_y = window_h // 2 - pic_h // 2 - 10
		piece_locations.append((piece, (piece_x, piece_y)))
		
	for piece_info in piece_locations:
		piece, location = piece_info
		window.scr.blit(piece, location)
		
	pygame.display.flip()
	
	piece = None
	while piece == None:
		for event in pygame.event.get():
			
			if event.type == pygame.QUIT:
				pygame.quit()
				sys.exit()
			
			elif event.type == pygame.MOUSEBUTTONDOWN:
				mouse_x, mouse_y = pygame.mouse.get_pos()
				choice = mouse_x // (window_w // 4)
				
				if choice == 0:
					piece = 'n'
				elif choice == 1:
					piece = 'b'
				elif choice == 2:
					piece = 'r'
				elif choice == 3:
					piece = 'q'
	
	piece = piece.upper() if (turn == 'w') else piece.lower()
	return piece

def try_user_move(fen, previous_board, piece_index, end_index):
	
	turn = fen.split(' ')[1]
	
	previous_board = bytearray(previous_board, 'utf-8')
	previous_board = (ctypes.c_char * len(previous_board))(*previous_board)
	
	board = make_board(fen)
	piece = board[piece_index]

	piece_x = piece_index%8
	piece_y = 7-piece_index//8

	end_x = end_index%8
	end_y = 7-end_index//8

	vec_x = end_x - piece_x
	vec_y = end_y - piece_y

	board_chars = bytearray([ord(str(square)) for square in board])
	board_chars = (ctypes.c_char * len(board_chars))(*board_chars)
	
	if engine.is_legal_move(board_chars, ord(piece), vec_x, vec_y, end_x, end_y, previous_board):
		
		previous_board = ''.join([str(square) for square in board])
		previous_board = bytearray(previous_board, 'utf-8')
		previous_board = (ctypes.c_char * len(previous_board))(*previous_board)
		
		turn = "b" if turn == "w" else "w"
		fen = get_new_fen(fen, piece_index, end_index)
		
		fen_chars = bytearray([ord(str(square)) for square in fen]) + b'\0' # fen is treated as char-table in made_move
		fen_chars = (ctypes.c_char * len(fen_chars))(*fen_chars)
		
		engine.made_move(fen_chars)
		
		board = make_board(fen)
		board_chars = bytearray([ord(str(square)) for square in board])
		board_chars = (ctypes.c_char * len(board_chars))(*board_chars)
		
		if engine.is_mate(board_chars, previous_board, ord(turn)):
			print("mate")
		elif engine.is_draw(board_chars, previous_board, ord(turn)):
			print("draw")
	
	previous_board = previous_board.value.decode('utf-8')
	return fen, previous_board, turn


fen_file_name = input("name of the fen-file (Note that if there exists a file with same name, it will be overwritten):\t")

window = MainScreen(800, 600)

turn = "w"
fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
previous_board = "rnbqkbnrpppppppp00000000000000000000000000000000PPPPPPPPRNBQKBNR"

IMAGES_BY_SYMBOL = import_images()
margin, treshold = 50, 500

piece_index = None

#fen = "4k3/8/8/8/8/8/8/4K3 w KQkq - 0 1"
#previous_board = "0000k0000000000000000000000000000000000000000000000000000000K000"

with open(fen_file_name, 'w') as fen_file:
	fen_file.write(fen)

while True:
	window.width, window.height = window.scr.get_size()
	
	for event in pygame.event.get():
		if event.type == pygame.QUIT:
			pygame.quit()
			sys.exit()
			
		elif event.type == pygame.MOUSEBUTTONDOWN:
			
			h = (window.height - margin*2 if (window.height > treshold) else treshold - margin*2)
			w = h
			board_area = pygame.Rect((window.width - w)/2, margin, w, h)
			
			mouse_pos = pygame.mouse.get_pos()
			if board_area.collidepoint(mouse_pos):
			
				if piece_index == None:
					piece_index = get_square_index(mouse_pos, margin, treshold)
					if get_color_by_index(piece_index, fen) != turn or piece_index == None:
						piece_index = None
						end_index = None
						continue
							
				else:
					end_index = get_square_index(mouse_pos, margin, treshold)
					if end_index == None:
						piece_index == None
						continue
					
					prev_fen = fen
					fen, previous_board, turn = try_user_move(fen, previous_board, piece_index, end_index)
					
					if prev_fen != fen:
						with open(fen_file_name, 'a') as fen_file:
							fen_file.write('\n' + fen)
					
					piece_index, end_index = None, None
			else:
				piece_index, end_index = None, None
			
			
	window.scr.fill((70,20,0))
	draw_board(margin, treshold)
	draw_pieces(fen, margin, treshold)
	pygame.display.update()
