include $(TOPDIR)/rules.mk

PKG_NAME:=mqtt_sub
PKG_RELEASE:=1
PKG_VERSION:=0.1.0
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk

define Package/mqtt_sub
	CATEGORY:=Utilities
	TITLE:=mqtt_sub
	DEPENDS:= +libmosquitto +libuci +libcurl +libjson-c +libsqlite3 +mqtt_sub_ctl 
endef

define Package/mqtt_sub/description
	A simple MQTT subscriber daemon program
endef

define Package/mqtt_sub/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_CONF) ./files/mqtt_sub.conf $(1)/etc/config/mqtt_sub
	$(INSTALL_BIN) ./files/mqtt_sub.init $(1)/etc/init.d/mqtt_sub
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqtt_sub $(1)/usr/sbin
endef

$(eval $(call BuildPackage,mqtt_sub))
