extends CanvasLayer

@onready var resume_button: Button = $CenterContainer/VBoxContainer/ResumeButton
@onready var disconnect_button: Button = $CenterContainer/VBoxContainer/DisconnectButton

func _ready() -> void:
	visible = false
	
	resume_button.pressed.connect(_on_resume_pressed)
	disconnect_button.pressed.connect(_on_disconnect_pressed)
	
func toggle_menu() -> void:
	visible = not visible
	
func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		toggle_menu()
		
func _on_resume_pressed() -> void:
	toggle_menu()
	
func _on_disconnect_pressed() -> void:
	get_tree().call_group("NetworkManager", "disconnect")
	get_tree().change_scene_to_file("res://scenes/main_menu.tscn")
