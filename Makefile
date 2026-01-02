include $(TOPDIR)/rules.mk

LUCI_TITLE:=Campus Network Auto Login
LUCI_DEPENDS:=+libcurl +libjson-c +libopenssl
LUCI_PKGARCH:=all

PKG_LICENSE:=GPL-2.0
PKG_MAINTAINER:=zeroxin <x1936999453@outlook.com>

include ../../luci.mk

define Package/luci-app-autologin/conffiles
/etc/config/autologin
endef

define Package/luci-app-autologin/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/autologin $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./root/etc/init.d/autologin $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./root/etc/config/autologin $(1)/etc/config/autologin
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./root/etc/uci-defaults/80_autologin $(1)/etc/uci-defaults/
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./root/usr/share/rpcd/acl.d/luci-app-autologin.json $(1)/usr/share/rpcd/acl.d/
	$(INSTALL_DIR) $(1)/usr/share/rpcd/ucode
	$(INSTALL_DATA) ./root/usr/share/rpcd/ucode/autologin.uc $(1)/usr/share/rpcd/ucode/
	$(INSTALL_DIR) $(1)/usr/share/luci/menu.d
	$(INSTALL_DATA) ./root/usr/share/luci/menu.d/luci-app-autologin.json $(1)/usr/share/luci/menu.d/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/autologin ./src/autologin.c \
		-lcurl -ljson-c -lssl -lcrypto
endef
