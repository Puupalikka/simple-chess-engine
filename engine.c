
/* TODO:
 * fix issues with checking for threefold repetition
 * check for 50-move-rule
 * implement the actual ai
 * call for ai's move when user has moved
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static const int white_p_vecs[][2]={{0,1},{0,2},{1,1},{-1,1}};
static const int black_p_vecs[][2] = {{0,-1},{0,-2},{1,-1},{-1,-1}};
static const int n_vecs[][2] = {{-2,1},{-1,2},{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1}};
static const int b_vecs[][2] = {
									{1,1},{2,2},{3,3},{4,4},{5,5},{6,6},{7,7},
									{-1,-1},{-2,-2},{-3,-3},{-4,-4},{-5,-5},{-6,-6},{-7,-7},
									{1,-1},{2,-2},{3,-3},{4,-4},{5,-5},{6,-6},{7,-7},
									{-1,1},{-2,2},{-3,3},{-4,4},{-5,5},{-6,6},{-7,7}
									};
static const int r_vecs[][2] = {
									{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{0,7},
									{0,-1},{0,-2},{0,-3},{0,-4},{0,-5},{0,-6},{0,-7},
									{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},
									{-1,0},{-2,0},{-3,0},{-4,0},{-5,0},{-6,0},{-7,0}
									};
static const int q_vecs[][2] = {
									{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{0,7},
									{0,-1},{0,-2},{0,-3},{0,-4},{0,-5},{0,-6},{0,-7},
									{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},
									{-1,0},{-2,0},{-3,0},{-4,0},{-5,0},{-6,0},{-7,0},
									{1,1},{2,2},{3,3},{4,4},{5,5},{6,6},{7,7},
									{-1,-1},{-2,-2},{-3,-3},{-4,-4},{-5,-5},{-6,-6},{-7,-7},
									{1,-1},{2,-2},{3,-3},{4,-4},{5,-5},{6,-6},{7,-7},
									{-1,1},{-2,2},{-3,3},{-4,4},{-5,5},{-6,6},{-7,7}
									};
static const int k_vecs[][2] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};

static const int p_size = 4;
static const int n_size = 8;
static const int b_size = 28;
static const int r_size = 28;
static const int q_size = 56;
static const int k_size = 8;

struct Position {
	char position[64];
	char turn;
	int ep_square;
	
	// 0 = x, 1 = w, 2 = b, 3 = w/b
	int castling_queen;
	int castling_king;
	struct Position* next;
};

struct CastlingVars {
	int white_king_rook;
	int white_queen_rook;
	int white_king;
	
	int black_king_rook;
	int black_queen_rook;
	int black_king;
};

struct CastlingVars mov_vars = {0,0,0,0,0,0};

static struct Position history_init = {
		.position = {
			'r','n','b','q','k','b','n','r',
			'p','p','p','p','p','p','p','p',
			'0','0','0','0','0','0','0','0',
			'0','0','0','0','0','0','0','0',
			'0','0','0','0','0','0','0','0',
			'0','0','0','0','0','0','0','0',
			'P','P','P','P','P','P','P','P',
			'R','N','B','Q','K','B','N','R'
			},
			
		.turn = 'w',
		.ep_square = -1,
		.castling_king = 0,
		.castling_queen = 0,
		.next = NULL
	};

static struct Position* history_end = &history_init;

int is_legal_vector(const char piece, const int vec_x, const int vec_y, const int end_x, const int end_y);
int ep_square(const char* prev_board, const char* curr_board);
int is_check(const char* board, char turn);
int is_basic_move_legal(const char* board, const char piece, const int vec_x, const int vec_y, const int end_x, const int end_y, const char* prev_board);
int is_castling_legal(struct CastlingVars mov_vars, const char* board, char turn, char side);
int is_legal_move(const char* board, const char piece, const int vec_x, const int vec_y, const int end_x, const int end_y, const char* prev_board);
int is_any_move_left(const char* board, const char* prev_board, char turn);
int is_mate(const char* board, const char* prev_board, char turn);
int is_draw(const char* board, const char* prev_board, char turn);
int is_enough_material(const char* board);
struct Position* append_to_history(const char* board, char turn, int ep_square, int castling_queen, int castling_king, struct Position *previous);
void free_history(struct Position *first);
int is_3fold_rep();
void board_from_fen(const char* fen, char* board);
void set_castling_rights(int* castlings, char* fen);
int is_ep_possible(char turn, char* board, int ep_index);
void print_board(char* board);
int get_ep_index(char* fen);
void made_move(char* fen);


// evaluating if move is legal
int is_legal_vector(const char piece, const int vec_x, const int vec_y, const int end_x, const int end_y){
	
	char pieces[] = {'P','p','N','n','B','b','R','r','Q','q','K','k'};
	
	int piece_found = 0;
	int len_pieces = sizeof(pieces) / sizeof(char);
	
	for (int i = 0; i < len_pieces; i++){
		if (pieces[i] == piece){
			piece_found = 1;
			break;
		}
	}
	
	if (!piece_found){
		printf("Move was discarded (can't find the chosen piece).\n");
		printf("piece: %c \n", piece);
		return 0;
	}
	
	int piece_x = end_x - vec_x;
	int piece_y = end_y - vec_y;
	
	if (piece_x + vec_x > 7 || piece_x + vec_x < 0 || piece_y + vec_y > 7 || piece_y + vec_y < 0) {
		printf("Piece was tried to move out of the board.");
		return 0;
	}
	
	switch (piece) {
		case 'p':
			for (int i = 0; i < p_size; i++){
				if (vec_x == black_p_vecs[i][0] && vec_y == black_p_vecs[i][1]){
					if (vec_y == -2 && end_y != 4){
						return 0;
					}
					return 1;
				}
			}
			return 0;
			break;
			
		case 'P':
			for (int i = 0; i < p_size; i++){
				if (vec_x == white_p_vecs[i][0] && vec_y == white_p_vecs[i][1]){
					if (vec_y == 2 && end_y != 3){
						return 0;
					}
					return 1;
				}
			}
			return 0;
			break;
			
		case 'n':
		case 'N':
			for (int i = 0; i < n_size; i++){
				if (vec_x == n_vecs[i][0] && vec_y == n_vecs[i][1]){
					return 1;
				}
			}
			return 0;
			break;
			
		case 'b':
		case 'B':
			for (int i = 0; i < b_size; i++){
				if (vec_x == b_vecs[i][0] && vec_y == b_vecs[i][1]){
					return 1;
				}
			}
			return 0;
			break;
			
		case 'r':
		case 'R':
			for (int i = 0; i < r_size; i++){
				if (vec_x == r_vecs[i][0] && vec_y == r_vecs[i][1]){
					return 1;
				}
			}
			return 0;
			break;
			
		case 'q':
		case 'Q':
			for (int i = 0; i < q_size; i++){
				if (vec_x == q_vecs[i][0] && vec_y == q_vecs[i][1]){
					return 1;
				}
			}
			return 0;
			break;
		
		case 'k':
		case 'K':
			for (int i = 0; i < k_size; i++){
				if (vec_x == k_vecs[i][0] && vec_y == k_vecs[i][1]){
					return 1;
				}
			}
			return 0;
			break;
		
		default:
			printf("Piece wasn't detected.\n");
			return 0;
	}
	return 0;
}

int ep_square(const char* prev_board, const char* curr_board){
	
	char previous_piece = '0';
	int previous_move_index, previous_piece_index, current_piece_index;
	
	for (int i = 0; i < 64; i++){
		
		if (prev_board[i] != curr_board[i]){
			if (prev_board[i] == '0'){
				previous_piece = curr_board[i];
				current_piece_index = i;
			}
			if (curr_board[i] == '0'){
				previous_piece = prev_board[i];
				previous_piece_index = i;
			}
		}
	}
	
	if (previous_piece == '0'){
		printf("Previous move wasn't detected while checking for rights to perform en passant (returning false).\n");
		return -1;
	}
	
	previous_move_index = current_piece_index - previous_piece_index;
	
	if (previous_piece == 'P' && previous_move_index == -16){
		return current_piece_index + 8;
		
	} else if (previous_piece == 'p' && previous_move_index == 16){
		return current_piece_index - 8;
		
	} else {
		return -1;
	}
}

int is_check(const char* board, char turn){
	
	int king_i = -1;
	if (turn == 'b'){
		for (int i = 0; i < 64; i++){
			if (board[i] == 'k'){
				king_i = i;
			}
		}
	} else if (turn == 'w') {
		for (int i = 0; i < 64; i++){
			if (board[i] == 'K'){
				king_i = i;
			}
		}
	}
	
	if (king_i == -1){
		printf("Could not find the king when calculating checks (returning 0).\n");
		return 0;
	}
	
	int new_i;
	
	static const int vec_up = -8;
	static const int vec_upright = -7;
	static const int vec_upleft = -9;
	static const int vec_right = 1;
	
	for (int i = 1; i < 8; i++){
		
		if (king_i%8 == 7 || king_i/8 == 0){
			break;
		}
		
		new_i = king_i + vec_upright * i;
		if (new_i%8 == 7 || new_i/8 == 0){
			if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	for (int i = 1; i < 8; i++){
		
		if (king_i%8 == 0 || king_i/8 == 0){
			break;
		}
		
		new_i = king_i + vec_upleft * i;
		if (new_i%8 == 0 || new_i/8 == 0){
			if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	for (int i = -1; i > -8; i--){
		
		if (king_i%8 == 0 || king_i/8 == 7){
			break;
		}
		
		new_i = king_i + vec_upright * i;
		if (new_i%8 == 0 || new_i/8 == 7){
			if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	for (int i = -1; i > -8; i--){
		
		if (king_i%8 == 7 || king_i/8 == 7){
			break;
		}
		
		new_i = king_i + vec_upleft * i;
		if (new_i%8 == 7 || new_i/8 == 7){
			if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'b' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'B' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	for (int i = 1; i < 8; i++){
		
		if (king_i/8 == 0){
			break;
		}
		
		new_i = king_i + vec_up * i;
		if (new_i/8 == 0){
			if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	for (int i = 1; i < 8; i++){
		
		if (king_i%8 == 7){
			break;
		}
		
		new_i = king_i + vec_right * i;
		if (new_i%8 == 7){
			if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	for (int i = -1; i > -8; i--){
		
		if (king_i/8 == 7){
			break;
		}
		
		new_i = king_i + vec_up * i;
		if (new_i/8 == 7){
			if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}

	for (int i = -1; i > -8; i--){
		
		if (king_i%8 == 0){
			break;
		}
		
		new_i = king_i + vec_right * i;
		if (new_i%8 == 0){
			if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
				return 1;
			} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
				return 1;
			}
			break;
		}
		
		if ((board[new_i] == 'r' || board[new_i] == 'q') && turn == 'w'){
			return 1;
		} else if ((board[new_i] == 'R' || board[new_i] == 'Q') && turn == 'b'){
			return 1;
		} else if (board[new_i] != '0'){
			break;
		}
	}
	
	int vec_x, vec_y;
	for (int i = 0; i < n_size; i++){
		vec_x = n_vecs[i][0];
		vec_y = n_vecs[i][1];
		new_i = king_i + vec_x + 8 * vec_y;
		
		if (new_i < 0 || new_i > 63){
			continue;
		}
		
		if (board[new_i] == 'n' && turn == 'w'){
			return 1;
		}
		if (board[new_i] == 'N' && turn == 'b'){
			return 1;
		}
	}
	
	if (turn == 'w'){
		if ((board[king_i - 8 + 1] == 'p' && king_i%8 != 7) || (board[king_i - 8 - 1] == 'p' && king_i%8 != 0)){
			return 1;
		}
	} else if (turn == 'b'){
		if ((board[king_i + 8 + 1] == 'P' && king_i%8 != 7) || (board[king_i + 8 - 1] == 'P' && king_i%8 != 0)){
			return 1;
		}
	} else {
		printf("Invalid turn detected when computing checks caused by pawn (returning false); turn: %c\n", turn);
		return 0;
	}
	
	int king_moves[] = {king_i+vec_upleft, king_i+vec_up, king_i+vec_upright, king_i+vec_right, king_i-vec_upleft, king_i-vec_up, king_i-vec_upright, king_i-vec_right};
	
	for (int i = 0; i < 8; i++){
		
		if (king_i%8 == 0 && (king_moves[i] == king_i+vec_upleft || king_moves[i] == king_i-vec_upright || king_moves[i] == king_i-vec_right)){
			continue;
		} else if (king_i%8 == 7 && (king_moves[i] == king_i+vec_upright || king_moves[i] == king_i-vec_upleft || king_moves[i] == king_i+vec_right)){
			continue;
		} else if (king_i/8 == 0 && (king_moves[i] == king_i+vec_upleft || king_moves[i] == king_i+vec_upright || king_moves[i] == king_i+vec_up)){
			continue;
		} else if (king_i/8 == 7 && (king_moves[i] == king_i-vec_upright || king_moves[i] == king_i-vec_upleft || king_moves[i] == king_i-vec_up)){
			continue;
		}
		
		if (board[king_moves[i]] == 'k' || board[king_moves[i]] == 'K'){
			return 1;
		}
	}
	
	return 0;
}

int is_basic_move_legal(const char* board, const char piece, const int vec_x, const int vec_y, const int end_x, const int end_y, const char* prev_board){
	
	int start_x = end_x-vec_x;
	int start_y = end_y-vec_y;
	int start_i = start_x+(7-start_y)*8;
	int end_i = end_x+(7-end_y)*8;
	
	if (end_x > 7 || end_x < 0 || end_y > 7 || end_y < 0){
		return 0;
	}
	
	if (isalpha(board[end_i])){
		if ((islower(board[end_i]) && islower(piece)) || (isupper(board[end_i]) && isupper(piece))){
			return 0;
		} else if (tolower(piece) == 'p' && vec_x == 0){
			return 0;
		}
	}
	
	char new_board[64];
	for (int i = 0; i < 64; i++){
		new_board[i] = board[i];
	}
	
	new_board[start_i] = '0';
	new_board[end_i] = piece;
	
	char turn = islower(piece) ? 'b' : 'w';
	int vector_is_legal = is_legal_vector(piece, vec_x, vec_y, end_x, end_y);
	int king_is_compromised = is_check(new_board, turn);
	
	if (!vector_is_legal || king_is_compromised){
		return 0;
	}
	
	if (tolower(piece) == 'n'){
		return 1;
	} else if (piece == 'P' && vec_y == 1 && (vec_x == -1 || vec_x == 1)) {
		if (board[end_i] != '0' || ep_square(prev_board, board) == end_i) {
			return 1;
		}
		return 0;
	} else if (piece == 'p' && vec_y == -1 && (vec_x == -1 || vec_x == 1)) {
		if (board[end_i] != '0' || ep_square(prev_board, board) == end_i) {
			return 1;
		}
		return 0;
	} else {
		
		int new_i;
		
		static const int vec_up = -8;
		static const int vec_upright = -7;
		static const int vec_upleft = -9;
		static const int vec_right = 1;
		
		
		if (vec_x == 0){
			if (vec_y > 0){
				for (int i = 1; i < 8; i++){
					new_i = start_i + vec_up*i;
					if (board[new_i] != '0' && new_i != end_i){
						return 0;
					} else if (new_i == end_i){
						return 1;
					}
				}
			} else {
				for (int i = -1; i > -8; i--){
					new_i = start_i + vec_up*i;
					if (board[new_i] != '0' && new_i != end_i){
						return 0;
					} else if (new_i == end_i){
						return 1;
					}
				}
			}
			
		} else if (vec_y == 0){
			if (vec_x > 0){
				for (int i = 1; i < 8; i++){
					new_i = start_i + vec_right*i;
					if (board[new_i] != '0' && new_i != end_i){
						return 0;
					} else if (new_i == end_i){
						return 1;
					}
				}
			} else {
				for (int i = -1; i < 8; i--){
					new_i = start_i + vec_right*i;
					if (board[new_i] != '0' && new_i != end_i){
						return 0;
					} else if (new_i == end_i){
						return 1;
					}
				}
			}
			
		} else if (vec_x > 0 && vec_y > 0){
			for (int i = 1; i < 8; i++){
				new_i = start_i + vec_upright*i;
				if (board[new_i] != '0' && new_i != end_i){
					return 0;
				} else if (new_i == end_i){
					return 1;
				}
			}
		} else if (vec_x > 0 && vec_y < 0){
			for (int i = -1; i < 8; i--){
				new_i = start_i + vec_upleft*i;
				if (board[new_i] != '0' && new_i != end_i){
					return 0;
				} else if (new_i == end_i){
					return 1;
				}
			}
		} else if (vec_x < 0 && vec_y < 0){
			for (int i = -1; i < 8; i--){
				new_i = start_i + vec_upright*i;
				if (board[new_i] != '0' && new_i != end_i){
					return 0;
				} else if (new_i == end_i){
					return 1;
				}
			}
		} else if (vec_x < 0 && vec_y > 0){
			for (int i = 1; i < 8; i++){
				new_i = start_i + vec_upleft*i;
				if (board[new_i] != '0' && new_i != end_i){
					return 0;
				} else if (new_i == end_i){
					return 1;
				}
			}
		}
		
		return 0;
	}
}

int is_castling_legal(struct CastlingVars mov_vars, const char* board, char turn, char side){
	
	if ((turn == 'b' && mov_vars.black_king != 0) || (turn == 'w' && mov_vars.white_king != 0) || is_check(board, turn)){
		return 0;
	}
	
	char new_board[64];
	for (int i = 0; i < 64; i++){
		new_board[i] = board[i];
	}
	
	if (turn == 'b'){
		if (side == 'k'){
			if (board[5] != '0' || board[6] != '0' || mov_vars.black_king_rook != 0 || board[7] != 'r') {
				return 0;
			} else {
				
				new_board[4] = '0';
				new_board[5] = 'k';
				if (is_check(new_board, 'b')){
					return 0;
				}
				
				new_board[5] = '0';
				new_board[6] = 'k';
				if (is_check(new_board, 'b')){
					return 0;
				}
			}
		} else {
			if (board[1] != '0' || board[2] != '0' || board[3] != '0' || mov_vars.black_queen_rook != 0 || board[0] != 'r') {
				return 0;
			} else {
				new_board[4] = '0';
				new_board[3] = 'k';
				if (is_check(new_board, 'b')){
					return 0;
				}
				
				new_board[3] = '0';
				new_board[2] = 'k';
				if (is_check(new_board, 'b')){
					return 0;
				}
			}
		}
		
	} else {
		if (side == 'k'){
			if (board[61] != '0' || board[62] != '0' || mov_vars.white_king_rook != 0 || board[63] != 'R') {
				return 0;
			} else {
				new_board[60] = '0';
				new_board[61] = 'K';
				if (is_check(new_board, 'w')){
					return 0;
				}
				
				new_board[61] = '0';
				new_board[62] = 'K';
				if (is_check(new_board, 'w')){
					return 0;
				}
			}
		} else {
			if (board[57] != '0' || board[58] != '0' || board[59] != '0' || mov_vars.white_queen_rook != 0 || board[59] != 'R') {
				return 0;
			} else {
				new_board[60] = '0';
				new_board[59] = 'K';
				if (is_check(new_board, 'w')){
					return 0;
				}
				
				new_board[59] = '0';
				new_board[58] = 'K';
				if (is_check(new_board, 'w')){
					return 0;
				}
			}
		}
	}
	
	return 1;
}

int is_legal_move(const char* board, const char piece, const int vec_x, const int vec_y, const int end_x, const int end_y, const char* prev_board){
	if (is_basic_move_legal(board, piece, vec_x, vec_y, end_x, end_y, prev_board)){
		
		if (piece == 'k'){
			mov_vars.black_king = 1;
			
		} else if (piece == 'K'){
			mov_vars.white_king = 1;
			
		} else if (piece == 'r' || piece == 'R'){
			
			int start_x = end_x - vec_x;
			int start_y = end_y - vec_y;
			int start_i = start_x + start_y*8;
			
			if (piece == 'r'){
				if (start_i == 0){
					mov_vars.black_queen_rook = 1;
				} else if (start_i == 7){
					mov_vars.black_king_rook = 1;
				}
				
			} else if (piece == 'R'){
				if (start_i == 56){
					mov_vars.white_queen_rook = 1;
				} else if (start_i == 63){
					mov_vars.white_king_rook = 1;
				}
			}
		}
		
		return 1;
		
	} else {
		
		if ((piece == 'k' || piece == 'K') && vec_y == 0 && (vec_x == 2 || vec_x == -2)){
			
			char turn = islower(piece) ? 'b' : 'w';
			char side = (vec_x == 2) ? 'k' : 'q';
			
			if (is_castling_legal(mov_vars, board, turn, side)){
				if (turn == 'b'){
					mov_vars.black_king = 1;
				} else {
					mov_vars.white_king = 1;
				}
				
				return 1;
			}
		}
		return 0;
	}
}

int is_any_move_left(const char* board, const char* prev_board, char turn){
	
	for (int i = 0; i < 64; i++){
		
		char piece = board[i];
		if ((turn == 'w' && islower(piece)) || (turn == 'b' && !islower(piece)) || piece == '0'){
			continue;
		}
		
		int start_x = i%8;
		int start_y = 7-i/8;
		
		if (piece == 'k' || piece == 'K'){
			for (int j = 0; j < k_size; j++){
				
				int vec_x = k_vecs[j][0];
				int vec_y = k_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
			
		} else if (piece == 'q' || piece == 'Q'){
			for (int j = 0; j < q_size; j++){
				
				int vec_x = q_vecs[j][0];
				int vec_y = q_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
			
		} else if (piece == 'r' || piece == 'R'){
			for (int j = 0; j < r_size; j++){
				
				int vec_x = r_vecs[j][0];
				int vec_y = r_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
			
		} else if (piece == 'b' || piece == 'B'){
			for (int j = 0; j < b_size; j++){
				
				int vec_x = b_vecs[j][0];
				int vec_y = b_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
			
		} else if (piece == 'n' || piece == 'N'){
			for (int j = 0; j < n_size; j++){
				
				int vec_x = n_vecs[j][0];
				int vec_y = n_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
			
		} else if (piece == 'p'){
			for (int j = 0; j < p_size; j++){
				
				int vec_x = black_p_vecs[j][0];
				int vec_y = black_p_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
			
		} else if (piece == 'P'){
			for (int j = 0; j < p_size; j++){
				
				int vec_x = white_p_vecs[j][0];
				int vec_y = white_p_vecs[j][1];
				int end_x = start_x + vec_x;
				int end_y = start_y + vec_y;
				
				if (is_basic_move_legal(board, (const char) piece, (const int) vec_x, (const int) vec_y, (const int) end_x, (const int) end_y, prev_board)){
					return 1;
				}
			}
		}
	}
	return 0;
}

int is_mate(const char* board, const char* prev_board, char turn){
	if (!is_any_move_left(board, prev_board, turn) && is_check(board, turn)){
		return 1;
	} else {
		return 0;
	}
}

int is_draw(const char* board, const char* prev_board, char turn){
	if (!is_any_move_left(board, prev_board, turn) && !is_check(board, turn)){
		return 1;
	} else if (!is_enough_material(board)){
		return 1;
	} else if (is_3fold_rep()){
		return 1;
	} else {
		return 0;
	}
}

int is_enough_material(const char* board){
	
	int bb_alive = 0;
	int bw_alive = 0;
	int n_alive = 0;
	
	for (int i = 0; i < 64; i++){
		
		char piece = board[i];
		
		if (piece == '0' || piece == 'k' || piece == 'K'){
			continue;
		} else if (piece == 'p' || piece == 'P' || piece == 'q' || piece == 'Q' || piece == 'r' || piece == 'R'){
			return 1;
		} else {
			if (piece == 'b' || piece == 'B'){
				if ((i/8+1%8)%2 == 0){
					bb_alive++;
				} else if ((i/8+1%8)%2 == 1){
					bw_alive++;
				}
			} else if (piece == 'n' || piece == 'N'){
				n_alive++;
			}
		}
	}
	
	if ((bb_alive > 0 && bw_alive > 0) || (bb_alive > 0 && n_alive > 0) || (bw_alive > 0 && n_alive > 0 ) || n_alive > 1){
		printf("bb: %d, bw: %d, n: %d\n", bb_alive, bw_alive, n_alive);
		return 1;
	} else {
		return 0;
	}
	
	printf("Something went wrong when calculating amount of material on the board (returning 1).\n");
	return 1;
}



// AI and history
void made_move(char* fen){ // doesn't call for ai's move (ai is yet to be implemented)
	
	char board[64];
	board_from_fen(fen, board);
	
	char turn = '-';
	int ep_index = -1;
	int castling_queen = 0;
	int castling_king = 0;
	int castlings[] = {castling_king, castling_queen};
	
	for (int i = 0; fen[i] != ' '; i++){
		turn = fen[i + 2];
	}
	
	set_castling_rights(castlings, fen);
	castling_king = castlings[0];
	castling_queen = castlings[1];
	ep_index = get_ep_index(fen);
	
	history_end = append_to_history((const char*) board, turn, ep_index, castling_queen, castling_king, history_end);
}

struct Position* append_to_history(const char* board, char turn, int ep_index, int castling_queen, int castling_king, struct Position *previous){
	struct Position *new_position = malloc(sizeof(struct Position));
	if (new_position == NULL){
		printf("Can't allocate memory for new board-position when updating history (leaving position unchanged).\n");
		return NULL;
	} else {
		for (int i = 0; i < 64; i++){
			new_position -> position[i] = (char) board[i];
		}
		
		new_position -> turn = turn;
		new_position -> ep_square = ep_index;
		new_position -> castling_queen = castling_queen;
		new_position -> castling_king = castling_king;
		new_position -> next = NULL;
		
		previous -> next = new_position;
		
		return new_position;
	}
}

void free_history(struct Position *first){
	
	if (first == NULL){
		struct Position* second = history_init.next;
		first = second;
	}
	
	struct Position* this_position;
	
	while (first != NULL){
		this_position = first;
		first = first->next;
		free(this_position);
	}
	
	history_init.next = NULL;
}

int is_3fold_rep(){ // doesn't work
	
	struct Position* this_history = &history_init;
	struct Position* last_history = history_end;
	
	const char* desired_position = last_history -> position;
	const char desired_turn = last_history -> turn;
	const int desired_castling_king = last_history -> castling_king;
	const int desired_castling_queen = last_history -> castling_queen;
	const int desired_ep_square = last_history -> ep_square;
		
	int repetitions = 0;
	
	while (this_history != NULL){
		
		int castling_match = (this_history->castling_king == desired_castling_king) && (this_history->castling_queen == desired_castling_queen);
		int ep_square_match = this_history->ep_square == desired_ep_square;
		int turn_match = this_history->turn == desired_turn;
		
		int rights_match = castling_match && ep_square_match && turn_match;
		
		if (rights_match && memcmp(this_history->position, desired_position, 64) == 0){
			repetitions++;
		}
		
		this_history = this_history->next;
	}
	
	if (repetitions >= 3){
		return 1;
	}
	
	return 0;
}

void set_castling_rights(int* castlings, char* fen){
	
	int castling_king = 0;
	int castling_queen = 0;
	
	int fen_len = strlen(fen);
	int spaces = 0;
	int castling_location = 1;
	
	int w_king = 0;
	int b_king = 0;
	int w_queen = 0;
	int b_queen = 0;
	
	for (int i = 1; spaces < 3; i++){
		if (fen[fen_len - i] == ' '){
			spaces++;
		}
		castling_location++;
	}
	
	for (int i = castling_location; fen[i] != ' '; i--){
		if (fen[i] == 'K'){
			w_king = 1;
		} else if (fen[i] == 'Q'){
			w_queen = 1;
		} else if (fen[i] == 'k'){
			b_king = 1;
		} else if (fen[i] == 'q'){
			b_queen = 1;
		}
	}
	
	if (w_king && b_king){
		castling_king = 3;
	} else if (w_king && !b_king){
		castling_king = 1;
	} else if (!w_king && b_king){
		castling_king = 2;
	} else if (!w_king && !b_king){
		castling_king = 0;
	}
	
	if (w_queen && b_queen){
		castling_queen = 3;
	} else if (w_queen && !b_queen){
		castling_queen = 1;
	} else if (!w_queen && b_queen){
		castling_queen = 2;
	} else if (!w_queen && !b_queen){
		castling_queen = 0;
	}
	
	castlings[0] = castling_king;
	castlings[1] = castling_queen;
}

int is_ep_possible(char turn, char* board, int ep_index){
	if (turn == 'b'){
		if (ep_index == 16){
			if (board[25] != 'P'){
				return 0;
			}
		} else if(ep_index == 23){
			if (board[30] != 'P'){
				return 0;
			}
		} else {
			if (board[ep_index + 7] != 'P' && board[ep_index + 9] != 'P'){
				return 0;
			}
		}
	} else {
		if (ep_index == 40){
			if (board[33] != 'p'){
				return 0;
			}
		} else if (ep_index == 47){
			if (board[38] != 'p'){
				return 0;
			}
		} else if (board[ep_index - 7] != 'P' && board[ep_index - 9] != 'P'){
			return 0;
		}
	}
	
	return 1;
}

int get_ep_index(char* fen){
	
	int fen_len = strlen(fen);
	int spaces = 0;
	int ep_location = 1;
	
	for (int i = 1; spaces < 2; i++){
		if (fen[fen_len - i] == ' '){
			spaces++;
		}
		ep_location++;
	}
	
	if (fen[fen_len - ep_location] != '-'){
		int ep_x = fen[fen_len - ep_location - 1] - 'a';
		int ep_y = fen[fen_len - ep_location] - '1';
		return (7-ep_y)*8 + ep_x;
	} else {
		return -1;
	}
}

void board_from_fen(const char* fen, char* board){
	int index = 63;
	for (int i = 0; fen[i] != ' '; i++){
		char letter = fen[i];
		if (letter != '/'){
			if (isalpha(letter)){
				board[63-index] = letter;
				index--;
			} else if (isdigit(letter)){
				int zeros = letter - '0';
				for (int j = 0; j < zeros; j++){
					board[63-index] = '0';
					index--;
				}
			} else {
				printf("Can't infer piece type correctly (leaving square empty).");
			}
		}
	}
}




// Here is some helper functions.
void print_board(char* board){
	printf("\nBOARD:\n");
	for (int y = 7; y >= 0; y--){
		for (int x = 0; x < 8; x++){
			printf("%c", board[(7-y)*8+x]);
		}
		printf("\n");
	}
}




	
	
	
	
	
	
	
	
	
	
	
	
	












