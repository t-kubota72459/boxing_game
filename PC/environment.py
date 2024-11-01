import paho.mqtt.client as mqtt
import PySimpleGUI as sg
import threading
import queue
import json
import pygame
from game_enum import Player, Status, Posture, create_payload
import time

message_queue = queue.Queue()
pygame.mixer.init()
pygame.mixer.music.load("ロッキーのテーマ曲.mp3")
pygame.mixer.music.set_volume(0.1)
pygame.mixer.music.play(-1)

gong_sound = {
    "START" : pygame.mixer.Sound("始め_ゴング.mp3"),
    "END" : pygame.mixer.Sound("終了_ゴング.mp3"),
}

# 各プレイヤーのパンチ数を管理する辞書
punch_counts = {Player.PLAYER1: 0, Player.PLAYER2: 0}

# ラウンドに関する変数
round_time = 30  # ラウンド時間 (秒)
current_round = 1
max_rounds = 3
start_time = 0
elapsed_time = 0
IN_FIGHT = False

# プログレスバーの更新間隔 (ミリ秒)
progress_bar_update_interval = 100

#
# コールバック関数: 接続が確立されたときに呼ばれる
#
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe("HTC_BOXING/STATUS")

def determin_loser(client, msg):
    player_id = Player(msg.payload[0])
    status = Status(msg.payload[1])
    posture = Posture(msg.payload[2])
    punches = msg.payload[3]

    if status == Status.FIGHT and (posture == Posture.FALLEN or punches >= 3):
        loser = player_id
        winner = Player(3 - player_id.value)
        client.publish("HTC_BOXING/COMMAND", bytes([winner.value, Status.WIN.value]))
        client.publish("HTC_BOXING/COMMAND", bytes([loser.value, Status.LOSE.value]))
        return winner

def ready(msg):
    player_id = Player(msg.payload[0])
    status = Status(msg.payload[1])
    posture = Posture(msg.payload[2])
    punches = msg.payload[3]

    if status != Status.IDLE or posture != Posture.STANDING or punches != 0:
        return False
    return True

#
# コールバック関数: メッセージが届いたときに呼ばれる
#
def on_message(client, userdata, msg):
    message_queue.put(msg)
#
# MQTTサブスクライバを実行するスレッド用関数
#
def mqtt_thread(client):
    # メッセージ受信を非同期で開始
    client.loop_start()

#
# GUIの実行
#
def change_background_color(color):
    window.TKroot.configure(bg=color)

def gui_thread(client):
    global start_time
    global elapsed_time
    global IN_FIGHT
    winner = None

    # ダークモード
    #sg.theme('DarkAmber')
    # 設定画面のレイアウト

    settings_layout = [
        [sg.Text('音量設定')],
        [sg.Text('BGM音量'), sg.Slider((0, 100), 50, orientation='h', size=(20, 20), key='-BGM_VOLUME-', enable_events=True)],
        [sg.Text('開始ゴング音量'), sg.Slider((0, 100), 50, orientation='h', size=(20, 20), key='-START_GONG_VOLUME-', enable_events=True)],
        [sg.Text('終了ゴング音量'), sg.Slider((0, 100), 50, orientation='h', size=(20, 20), key='-END_GONG_VOLUME-', enable_events=True)],
    ]

    log_layout = [
        [ sg.Multiline(size=(50, 10), key="-OUTPUT-") ]
    ]

# 画面レイアウトのデザイン
    game_layout = [
        [
            sg.Column([
                    [
                    sg.Image('img/olympic12_boxing.png'),
                    sg.Text("広島テックボクシング🥊", font=("メイリオ", 30, 'bold'), justification='center',
                        text_color='yellow', background_color='darkred', pad=(10, 20))
                    ]
                ], justification='center')
        ],
        [
            sg.Column([
                [
                    sg.Text("プレイヤー1のヒット数:", font=("メイリオ", 20), text_color='white', background_color='blue', pad=(10, 10, 10, 10)),
                    sg.Text("0", font=("メイリオ", 40, 'bold'), text_color='yellow', background_color='black', size=(5, 1), key='-PLAYER1-HIT-', pad=(10, 10, 10, 10))
                ],
                [
                    sg.Text("プレイヤー2のヒット数:", font=("メイリオ", 20), text_color='white', background_color='blue', pad=(10, 10, 10, 10)),
                    sg.Text("0", font=("メイリオ", 40, 'bold'), text_color='yellow', background_color='black', size=(5, 1), key='-PLAYER2-HIT-', pad=(10, 10, 10, 10))
                ]
            ], element_justification='center', pad=(0, 20))
        ],
        [
            sg.Button('🥊 FIGHT', font=("メイリオ", 18), button_color=('white', 'red'), size=(10, 2), disabled=True, pad=(10, 10)),
            sg.Button('RESET', font=("メイリオ", 18), button_color=('black', 'lightgrey'), size=(10, 2), pad=(10, 10))
        ],
        [
            sg.ProgressBar(round_time, orientation='h', size=(30, 30), key='-PROGRESSBAR-', bar_color=('red', 'black'), pad=(0, 20))
        ],
        [
            sg.Button("Exit", font=("メイリオ", 15), button_color=('white', 'darkblue'), size=(10, 1), pad=(0, 20))
        ]
    ]

    tab_group = [[sg.Tab('試合', game_layout), sg.Tab('設定', settings_layout), sg.Tab('ログ', log_layout)]]
    layout = [
        [  sg.TabGroup( tab_group ) ]
    ]

    window = sg.Window("Hiroshima Tech Boxing Game", layout, resizable=True)
    window.read(timeout=progress_bar_update_interval)
    window['-PROGRESSBAR-'].update_bar(100)

    #
    # PySimpleGUI のイベントループ
    #
    while True:
        event, values = window.read(timeout=progress_bar_update_interval)
        if event == sg.WINDOW_CLOSED or event == "Exit":
            break
    
        # メッセージキューからデータを取得して表示
        try:
            while not message_queue.empty():
                msg = message_queue.get_nowait()  # get message from queue
                player_id = Player(msg.payload[0])
                status = Status(msg.payload[1])
                posture = Posture(msg.payload[2]) 
                punches = msg.payload[3]
                punch_counts[player_id] = punches                

                window["-OUTPUT-"].print(f"{player_id} is {status}, {posture}, {punches}")
                window['-PLAYER1-HIT-'].update(f"　{punch_counts[Player.PLAYER2]}")   ## くらった数なので、表示が逆になる！
                window['-PLAYER2-HIT-'].update(f"　{punch_counts[Player.PLAYER1]}")   ## くらった数なので、表示が逆になる！

                if IN_FIGHT:
                    winner = determin_loser(client, msg)
                else:
                    if not ready(msg):
                        sg.popup_ok(
                            f"プレイヤー {player_id} が準備できてません。",
                            title="まって！", 
                            font=("メイリオ", 40, "bold"),  # メッセージのフォントサイズを大きく設定
                            button_color=('white', 'blue'),  # OKボタンの色設定
                            keep_on_top=True,  # 常に最前面に表示
                            location=(None, None),  # 画面中央に表示
                            #size=(10, 1)  # OKボタンを小さく設定
                        )
                        window["🥊 FIGHT"].update(disabled=True)
        except queue.Empty:
            pass

        #
        # 時間計測
        #
        if IN_FIGHT:
            current_time = time.time()
            elapsed_time = current_time - start_time
            window['-PROGRESSBAR-'].update_bar(round_time - elapsed_time)  # elapsed_timeは経過時間を表す変数
        
            if elapsed_time >= round_time:
                # DRAWメッセージ送信
                for player in Player:
                    payload = create_payload(player, Status.DRAW)
                    client.publish("HTC_BOXING/COMMAND", payload)
                
                gong_sound["END"].play()
                IN_FIGHT = False
                start_time = 0

                # 引き分けを宣言
                sg.popup_ok(
                    "引き分けです！",
                    title="試合終了", 
                    font=("メイリオ", 40, "bold"),  # メッセージのフォントサイズを大きく設定
                    button_color=('white', 'blue'),  # OKボタンの色設定
                    keep_on_top=True,  # 常に最前面に表示
                    location=(None, None),  # 画面中央に表示
                    #size=(10, 1)  # OKボタンを小さく設定
                )
        #
        # 勝者が確定した
        #
        if winner:
            sg.popup_ok(
                f"勝者: プレイヤー{winner.value}!", 
                title="試合終了", 
                font=("メイリオ", 40, "bold"),  # メッセージのフォントサイズを大きく設定
                button_color=('white', 'blue'),  # OKボタンの色設定
                keep_on_top=True,  # 常に最前面に表示
                location=(None, None),  # 画面中央に表示
                #size=(10, 1)  # OKボタンを小さく設定
            )
            gong_sound["END"].play()
            IN_FIGHT = False
            start_time = 0
            winner = None
    
        # RESET ボタンが押された時の処
        if event == "RESET":
            IN_FIGHT = False
            start_time = 0
            winner = None
            window['-PROGRESSBAR-'].update_bar(100)
            for player in Player:
                payload = create_payload(player, Status.READY)
                client.publish("HTC_BOXING/COMMAND", payload)
            window["🥊 FIGHT"].update(disabled=False)


        # FIGHTボタンが押された時の処理
        if event == "🥊 FIGHT":
            start_time = time.time()
            IN_FIGHT = True
            gong_sound["START"].play()
            for player in Player:
                payload = create_payload(player, Status.FIGHT)
                client.publish("HTC_BOXING/COMMAND", payload)
            window["🥊 FIGHT"].update(disabled=True)

        if event == '-BGM_VOLUME-':
            volume = values['-BGM_VOLUME-'] / 100.0
            pygame.mixer.music.set_volume(volume)
        elif event == '-START_GONG_VOLUME-':
            volume = values['-START_GONG_VOLUME-'] / 100.0
            gong_sound["START"].set_volume(volume)
        elif event == '-END_GONG_VOLUME-':
            volume = values['-END_GONG_VOLUME-'] / 100.0
            gong_sound["END"].set_volume(volume)
            
    window.close()

# メイン関数
def main():
    # MQTTクライアントのインスタンスを作成
    client = mqtt.Client()

    # コールバックを設定
    client.on_connect = on_connect
    client.on_message = on_message

    # MQTTブローカーに接続
    broker = "localhost"
    port = 1883
    client.connect(broker, port, 60)

    # MQTTスレッドを開始
    mqtt_thread_instance = threading.Thread(target=mqtt_thread, args=(client,))
    mqtt_thread_instance.start()

    # 起動時に RESET 要請
    for player in Player:
        payload = create_payload(player, Status.READY)
        client.publish("HTC_BOXING/COMMAND", payload)

    # GUIをメインスレッドで実行
    gui_thread(client)

    # ウィンドウが閉じられたら MQTT ループを停止し、接続を切断
    client.loop_stop()
    client.disconnect()

    # MQTTスレッドの終了を待つ
    mqtt_thread_instance.join()

# メイン関数の実行
if __name__ == "__main__":
    main()
