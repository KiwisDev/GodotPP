extends Node

@onready var network_manager: NetworkManager = $NetworkManager

func _ready() -> void:
	var ip = GameInstance.target_ip
	
	if not ip.is_empty():
		network_manager.try_connect(ip)
