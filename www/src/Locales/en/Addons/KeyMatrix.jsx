export default {
	'header-text': 'Keyboard Matrix',
	'sub-header-text':
		'Wire buttons as a keyboard-style row and column matrix to get more inputs from the same pins. Each switch connects one row pin to one column pin (not to ground) and needs its own diode, stripe toward the row, for full N-key rollover.',
	'pin-note':
		'Matrix switches act like extra input pins. The switches use the bits of the row and column pins first, then pins {{firstVirtual}} to 63. Switch mappings are shared by all profiles. Save and reboot after changing rows or columns.',
	'row-pins-label': 'Row pins',
	'col-pins-label': 'Column pins',
	'pins-placeholder': 'e.g. 16, 17, 18',
	'settle-label': 'Settle time (µs)',
	'switch-count': '{{rows}} rows × {{cols}} columns = {{count}} switches',
	'switch-count-capped':
		'{{rows}} rows × {{cols}} columns = {{wanted}} switches, but only {{count}} fit. The rest are ignored.',
	'switch-label': 'R{{row}} C{{col}}',
	'pin-label': 'pin {{pin}}',
	'save-label': 'Save matrix',
	'invalid-pins': 'Use a comma-separated list of GPIO numbers.',
};
