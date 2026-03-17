extends Control

@onready var ip_input: LineEdit = $IP_Input
@onready var connect_button: Button = $ConnectButton

func _ready() -> void:
	connect_button.pressed.connect(_on_connect_button_pressed)
	
func _on_connect_button_pressed() -> void:
	var entered_ip: String = ip_input.text.strip_edges()
	
	if entered_ip.is_empty():
		print("Error : IP Field is empty")
		return
		
	GameInstance.target_ip = entered_ip
	
	var error: Error = get_tree().change_scene_to_file("res://scenes/game.tscn")
	
	if error != OK:
		print("Error : Failed to load scene : ", error)
