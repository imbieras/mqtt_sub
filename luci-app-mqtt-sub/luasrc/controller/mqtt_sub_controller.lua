module("luci.controller.mqtt_sub_controller", package.seeall)

function index()
	entry({ "admin", "services", "mqtt" }, firstchild(), _("MQTT"), 150)
	entry({ "admin", "services", "mqtt", "subscriber" }, cbi("mqtt_sub"), _("Subscriber"), 3).leaf = true
end
