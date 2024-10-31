import paho.mqtt.client as mqtt
import PySimpleGUI as sg
import threading
import queue
import json
import pygame
from game_enum import Player, Status, Posture, create_payload

message_queue = queue.Queue()
pygame.mixer.init()
pygame.mixer.music.load("ロッキーのテーマ曲.mp3")
pygame.mixer.music.set_volume(0.1)
pygame.mixer.music.play(-1)

gong_sound = {
    "START" : pygame.mixer.Sound("始め_ゴング.mp3"),
    "END" : pygame.mixer.Sound("終了_ゴング.mp3"),
}

#
# コールバック関数: 接続が確立されたときに呼ばれる
#
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe("HTC_BOXING/STATUS")

#
# コールバック関数: メッセージが届いたときに呼ばれる
#
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

last_message = None
def on_message(client, userdata, msg):
    global last_message

    print(msg)
    print(last_message)
    print(msg.payload)
    if last_message is not None and msg.payload == last_message.payload:
        return

    player_id = Player(msg.payload[0])
    status = Status(msg.payload[1])
    posture = Posture(msg.payload[2])
    punches = msg.payload[3]

    if status == Status.FIGHT:
        winner = determin_loser(client, msg)
        message_queue.put(["WIN", winner])
    
    last_message = msg
#
# MQTTサブスクライバを実行するスレッド用関数
#
def mqtt_thread(client):
    # メッセージ受信を非同期で開始
    client.loop_start()

#
# GUIの実行
#
message_dict = {
    "FIGHT" : b"\x01\x02",
    "WIN"   : b"\x01\x03",
    "DRAW"  : b"\x01\x04",
    "LOSE"  : b"\x01\x05",
    "RESET" : b"\x01\x07",
}

def gui_thread(client):
    layout = [
        [sg.Text("MQTT Subscriber and Publisher")],
        [sg.Button('FIGHT'), sg.Button('RESET')],
        [sg.Multiline(size=(50, 10), key="-OUTPUT-")],
        [sg.Button("Exit")]
    ]

    window = sg.Window("MQTT Subscriber & Publisher", layout)

    #
    # PySimpleGUI のイベントループ
    #
    while True:
        event, values = window.read(timeout=50)  # 100msごとにウィンドウを更新
        if event == sg.WINDOW_CLOSED or event == "Exit":
            break
    
        # メッセージキューからデータを取得して表示
        try:
            while not message_queue.empty():
                msg = message_queue.get_nowait()  # get message from queue
                window["-OUTPUT-"].print(f"{msg}")
                gong_sound["END"].play()

        except queue.Empty:
            pass

        # RESET ボタンが押された時の処
        if event == "RESET":
            for player in Player:
                payload = create_payload(player, Status.READY)
                client.publish("HTC_BOXING/COMMAND", payload)

        # FIGHTボタンが押された時の処理
        if event == "FIGHT":
            gong_sound["START"].play()
            for player in Player:
                payload = create_payload(player, Status.FIGHT)
                client.publish("HTC_BOXING/COMMAND", payload)

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

