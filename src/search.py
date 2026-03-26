import evaluate
import moves as _moves

INF = 100_000


def negamax(board, depth, alpha, beta):
    if depth == 0:
        return evaluate.evaluate(board)
    
    moves = _moves.generate_moves(board)
    legal = 0

    for move in moves:
        saved = board.save_state()
        _moves.make_move(board, move)
        board.next_turn()

        if board.is_in_check(board.turn ^ 1):  # illegal
            board.restore_state(saved)
            continue

        score = -negamax(board, depth -1, -beta, -alpha)
        board.restore_state(saved)
        legal += 1

        if score >= beta: # prune
            return beta

        alpha = max(score, alpha)

    if legal == 0: # checkmate or stalemate
        return -INF if board.is_in_check(board.turn) else 0 
    
    return alpha
        

def best_move(board, depth):
    moves = _moves.generate_moves(board)
    best = None
    alpha = -INF

    for move in moves:
        saved = board.save_state()
        _moves.make_move(board, move)
        board.next_turn()

        if board.is_in_check(board.turn ^ 1): # illegal move
            board.restore_state(saved)
            continue

        score = -negamax(board, depth - 1, -INF, -alpha)
        board.restore_state(saved)

        if score > alpha:
            alpha = score
            best = move

    return best


    