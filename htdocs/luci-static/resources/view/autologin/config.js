'use strict';
'require form';
'require uci';
'require view';
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
		var m, s, o;

		m = new form.Map('autologin', _('Campus Network Auto Login'),
			_('Automatic login for campus network using Srun authentication system'));

		s = m.section(TypedSection, 'config', _('Configuration'));
		s.anonymous = true;
		s.addremove = false;

		o = s.option(form.Flag, 'enabled', _('Enable'));
		o.default = '0';

		o = s.option(form.Value, 'username', _('Username'));
		o.rmempty = false;

		o = s.option(form.Value, 'password', _('Password'));
		o.password = true;
		o.rmempty = false;

		o = s.option(form.ListValue, 'domain', _('ISP'));
		o.value('', _('Campus Network'));
		o.value('@cmcc', _('China Mobile'));
		o.value('@unicom', _('China Unicom'));
		o.value('@telecom', _('China Telecom'));
		o.default = '';

		o = s.option(form.Value, 'server', _('Authentication Server'));
		o.default = '172.26.8.11';
		o.rmempty = false;

		o = s.option(form.Value, 'ac_id', _('AC ID'));
		o.default = '1';

		o = s.option(form.Flag, 'use_https', _('Use HTTPS'));
		o.default = '0';

		o = s.option(form.Flag, 'auto_start', _('Auto Start'));
		o.default = '0';

		return m.render();
	}
});
