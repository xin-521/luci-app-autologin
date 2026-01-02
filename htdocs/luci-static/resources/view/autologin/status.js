'use strict';
'require view';
'require uci';
'require rpc';

var callGetStatus = rpc.declare({
	object: 'luci.autologin',
	method: 'getStatus',
	expect: { '': {} }
});

var callDoLogin = rpc.declare({
	object: 'luci.autologin',
	method: 'doLogin',
	params: [],
	expect: { '': {} }
});

return view.extend({
	load: function() {
		return Promise.all([
			uci.load('autologin')
		]);
	},

	render: function() {
		var statusDiv = E('div', { 'class': 'cbi-section' }, [
			E('h2', _('Status')),
			E('div', { 'class': 'cbi-section-node' }, [
				E('div', { 'class': 'cbi-value' }, [
					E('label', { 'class': 'cbi-value-title' }, _('Current Status')),
					E('div', { 'class': 'cbi-value-field' }, [
						E('span', { 'id': 'status' }, _('Loading...'))
					])
				]),
				E('div', { 'class': 'cbi-value' }, [
					E('label', { 'class': 'cbi-value-title' }, _('Actions')),
					E('div', { 'class': 'cbi-value-field' }, [
						E('button', {
							'class': 'cbi-button cbi-button-apply',
							'id': 'login-btn',
							'click': this.doLogin.bind(this)
						}, _('Login Now'))
					])
				])
			])
		]);

		this.updateStatus();
		this.statusInterval = setInterval(this.updateStatus.bind(this), 5000);

		return statusDiv;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null,

	updateStatus: function() {
		var self = this;
		callGetStatus().then(function(data) {
			var statusEl = document.getElementById('status');
			if (statusEl && data && data.status) {
				statusEl.textContent = data.status;
			}
		}).catch(function(err) {
			var statusEl = document.getElementById('status');
			if (statusEl) {
				statusEl.textContent = _('Error: ') + (err.message || _('Unknown error'));
			}
		});
	},

	doLogin: function() {
		var self = this;
		var btn = document.getElementById('login-btn');
		if (btn) {
			btn.disabled = true;
			btn.textContent = _('Logging in...');
		}

		callDoLogin().then(function(data) {
			if (btn) {
				btn.disabled = false;
				btn.textContent = _('Login Now');
			}

			if (data && data.success) {
				alert(_('Login successful'));
			} else {
				alert(_('Login failed: ') + (data && data.message ? data.message : _('Unknown error')));
			}

			self.updateStatus();
		}).catch(function(err) {
			if (btn) {
				btn.disabled = false;
				btn.textContent = _('Login Now');
			}

			alert(_('Login failed: ') + (err.message || _('Unknown error')));
			self.updateStatus();
		});
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
