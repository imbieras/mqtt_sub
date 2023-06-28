m = Map(
	"mqtt_sub",
	translate("MQTT Subscriber"),
	translate(
		"An MQTT subscriber is a client application or device that connects to an MQTT broker to receive and consume messages published on specific topics."
	)
)
local certs = require("luci.model.certificate")
local s = m:section(NamedSection, "mqtt_sub", "mqtt_sub", translate(""), translate(""))

enabled_sub = s:option(Flag, "enabled", translate("Enable"), translate("Select to enable MQTT subscriber"))

host = s:option(Value, "host", translate("Hostname"), translate("Specify address of the broker"))
host:depends("enabled", "1")
host.placeholder = "www.example.com"
host.datatype = "host"
host.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: hostname is empty")
	end
	Value.parse(self, section, novld, ...)
end

port = s:option(Value, "port", translate("Port"), translate("Specify port of the broker"))
port:depends("enabled", "1")
port.default = "1883"
port.placeholder = "1883"
port.datatype = "port"
port.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local value = self:formvalue(section)
	if enabled and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: port is empty")
	end
	Value.parse(self, section, novld, ...)
end

username = s:option(Value, "username", translate("Username"), translate("Specify username of remote host"))
username.datatype = "credentials_validate"
username.placeholder = translate("Username")
username:depends("enabled", "1")
username.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local pass = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.password")
	local value = self:formvalue(section)
	if enabled and pass ~= nil and pass ~= "" and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: username is empty but password is not")
	end
	Value.parse(self, section, novld, ...)
end

password = s:option(
	Value,
	"password",
	translate("Password"),
	translate("Specify password of remote host. Allowed characters (a-zA-Z0-9!@#$%&*+-/=?^_`{|}~. )")
)
password:depends("enabled", "1")
password.password = true
password.datatype = "credentials_validate"
password.placeholder = translate("Password")
password.parse = function(self, section, novld, ...)
	local enabled = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.enabled")
	local user = luci.http.formvalue("cbid.mqtt_sub.mqtt_sub.username")
	local value = self:formvalue(section)
	if enabled and user ~= nil and user ~= "" and (value == nil or value == "") then
		self:add_error(section, "invalid", "Error: password is empty but username is not")
	end
	Value.parse(self, section, novld, ...)
end

FileUpload.size = "262144"
FileUpload.sizetext = translate("Selected file is too large, max 256 KiB")
FileUpload.sizetextempty = translate("Selected file is empty")
FileUpload.unsafeupload = true

tls_enabled = s:option(Flag, "tls", translate("TLS"), translate("Select to enable TLS encryption"))
tls_enabled:depends("enabled", "1")

tls_type = s:option(ListValue, "tls_type", translate("TLS Type"), translate("Select the type of TLS encryption"))
tls_type:depends({ enabled = "1", tls = "1" })
tls_type:value("cert", translate("Certificate based"))
tls_type:value("psk", translate("Pre-Shared-Key based"))

tls_insecure = s:option(
	Flag,
	"tls_insecure",
	translate("Allow insecure connection"),
	translate("Allow not verifying server authenticity")
)
tls_insecure:depends({ enabled = "1", tls = "1", tls_type = "cert" })

local certificates_link = luci.dispatcher.build_url("admin", "system", "admin", "certificates")
o = s:option(
	Flag,
	"_device_files",
	translate("Certificate files from device"),
	translatef(
		"Choose this option if you want to select certificate files from device.\
																					Certificate files can be generated <a class=link href=%s>%s</a>",
		certificates_link,
		translate("here")
	)
)
o:depends({ tls = "1", tls_type = "cert" })
local cas = certs.get_ca_files().certs
local certificates = certs.get_certificates()
local keys = certs.get_keys()

tls_cafile = s:option(FileUpload, "cafile", translate("CA file"), "")
tls_cafile:depends({ tls = "1", _device_files = "", tls_type = "cert" })

tls_certfile = s:option(FileUpload, "certfile", translate("Certificate file"), "")
tls_certfile:depends({ tls = "1", _device_files = "", tls_type = "cert" })

tls_keyfile = s:option(FileUpload, "keyfile", translate("Key file"), "")
tls_keyfile:depends({ tls = "1", _device_files = "", tls_type = "cert" })

tls_cafile = s:option(ListValue, "_device_cafile", translate("CA file"), "")
tls_cafile:depends({ tls = "1", _device_files = "1", tls_type = "cert" })

if #cas > 0 then
	for _, ca in pairs(cas) do
		tls_cafile:value("/etc/certificates/" .. ca.name, ca.name)
	end
else
	tls_cafile:value("", translate("-- No files available --"))
end

function tls_cafile.write(self, section, value)
	m.uci:set(self.config, section, "cafile", value)
end

tls_cafile.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "cafile") or ""
end

tls_certfile = s:option(ListValue, "_device_certfile", translate("Certificate file"), "")
tls_certfile:depends({ tls = "1", _device_files = "1", tls_type = "cert" })

if #certificates > 0 then
	for _, certificate in pairs(certificates) do
		tls_certfile:value("/etc/certificates/" .. certificate.name, certificate.name)
	end
else
	tls_certfile:value("", translate("-- No files available --"))
end

function tls_cafile.write(self, section, value)
	m.uci:set(self.config, section, "certfile", value)
end

tls_cafile.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "certfile") or ""
end

tls_keyfile = s:option(ListValue, "_device_keyfile", translate("Key file"), "")
tls_keyfile:depends({ tls = "1", _device_files = "1", tls_type = "cert" })

if #keys > 0 then
	for _, key in pairs(keys) do
		tls_keyfile:value("/etc/certificates/" .. key.name, key.name)
	end
else
	tls_keyfile:value("", translate("-- No files available --"))
end

function tls_keyfile.write(self, section, value)
	m.uci:set(self.config, section, "keyfile", value)
end

tls_keyfile.cfgvalue = function(self, section)
	return m.uci:get(m.config, section, "keyfile") or ""
end

o = s:option(
	Value,
	"psk",
	translate("Pre-Shared-Key"),
	translate("The pre-shared-key in hex format with no leading “0x”")
)
o.datatype = "lengthvalidation(0, 128)"
o.placeholder = "Key"
o:depends({ tls = "1", tls_type = "psk" })

o = s:option(Value, "psk_identity", translate("Identity"), translate("Specify the Identity"))
o.datatype = "uciname"
o.placeholder = "Identity"
o:depends({ tls = "1", tls_type = "psk" })

return m
