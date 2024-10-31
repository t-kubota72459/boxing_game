from enum import Enum

class Player(Enum):
    PLAYER1 = 1
    PLAYER2 = 2

class Posture(Enum):
    STANDING = 1
    FALLEN = 2

class Status(Enum):
    IDLE = 1
    FIGHT = 2
    WIN = 3
    DRAW = 4
    LOSE = 5
    BUSY = 6
    READY = 7

def create_payload(player: Player, status: Status) -> bytes:
    """
    プレイヤーとステータスから2バイトのペイロードを作成する

    Args:
        player: プレイヤー
        status: ステータス

    Returns:
        bytes: 2バイトのペイロード
    """
    return bytes([player.value, status.value])
