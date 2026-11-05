export default {
	'header-text': 'Keyboard Matrix',
	'sub-header-text':
		'Wire buttons as a keyboard-style row and column matrix to get more inputs from the same pins. Each switch connects one row pin to one column pin (not to ground) and needs its own diode, stripe toward the row, for full N-key rollover.',
	'pin-note':
		'Matrix switches act like extra input pins. The switches use the bits of the row and column pins first, then pins {{firstVirtual}} to 63. Each profile can map the switches its own way. Save and reboot after changing rows or columns.',
	'row-pins-label': 'Row pins',
	'col-pins-label': 'Column pins',
	'pins-placeholder': 'e.g. 16, 17, 18',
	'settle-label': 'Settle time (µs)',
	'row-delay-label': 'Row delay (µs)',
	'timing-note':
		'Settle time: how long each row is pulled low before the columns are read. Row delay: pause after each row before the next one, so long wiring can recover. Raise the row delay if a switch shows phantom presses on the next row. A full scan takes rows × (settle + row delay + about 1 µs).',
	'switch-count': '{{rows}} rows × {{cols}} columns = {{count}} switches',
	'switch-count-capped':
		'{{rows}} rows × {{cols}} columns = {{wanted}} switches, but only {{count}} fit. The rest are ignored.',
	'switch-label': 'R{{row}} C{{col}}',
	'pin-label': 'pin {{pin}}',
	'profile-label': 'Profile',
	'profile-option': 'Profile {{number}}',
	'profile-disabled': 'disabled',
	'profile-custom-label':
		"Own switch mappings for this profile (off: uses {{base}}'s)",
	'save-label': 'Save matrix',
	'invalid-pins': 'Use a comma-separated list of GPIO numbers.',
};
