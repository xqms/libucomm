
from pyparsing import *

BUILTIN_TYPES = {
	'uint8_t': 1,
	'int8_t': 1,
	'uint16_t': 2,
	'int16_t': 2,
	'uint32_t': 4,
	'int32_t': 4,
}

def registerParseAction(cls):
	cls.grammar.setParseAction(cls.parse)

class UnknownTypeError(Exception):
	def __init__(self, type):
		super(UnknownTypeError, self).__init__("Unknown type '%s'" % type)

		self.type = type

class Identifier:
	grammar = Word(alphas, alphanums + '_')

class EnumEntry:
	grammar = (
		  Identifier.grammar("name")
		+ Optional(Suppress("=") + SkipTo(Literal("}") | Literal(","), include=False)("assignement"))
	)

	def __init__(self, name, assignement=None):
		self.name = name
		self.assignement = assignement

	def definition(self):
		if self.assignement:
			return str(self.name) + " = " + str(self.assignement) + ","
		else:
			return str(self.name) + ","

	@classmethod
	def parse(cls, parse_result):
		assignement = None

		if parse_result.assignement:
			assignement = parse_result.assignement

		return cls(parse_result.name, assignement)
registerParseAction(EnumEntry)


class Enum:
	grammar = (
		  Suppress("enum")
		+ Optional(Identifier.grammar)("name")
		+ Suppress("{")
		+ (
			  ZeroOrMore( EnumEntry.grammar + Suppress(Literal(",")))
			+ Optional(EnumEntry.grammar)
		  )("entries")
		+ Suppress("}")
		+ Suppress(";")
	)

	def __init__(self, name, entries):
		self.name = name
		self.entries = list(entries)

	def __str__(self):
		return self.name

	def definition(self, indent_level = 0):
		base_indent = '\t' * indent_level
		code = []
		if self.name:
			code.append(base_indent + 'enum %s' % self.name)
		else:
			code.append(base_indent + 'enum')
		code += [
			base_indent + '{',
			'\n'.join([base_indent + '\t' + e.definition() for e in self.entries]),
			base_indent + '};'
		]

		return '\n'.join(code)

	@classmethod
	def parse(cls, parse_result):
		return cls(parse_result.name, parse_result.entries)
registerParseAction(Enum)


class Member:
	grammar = (
		  Identifier.grammar("type")
		+ Identifier.grammar("name")
		+ Optional(Literal('[') + Optional(CharsNotIn(']'))("size") + Literal(']'))("array")
		+ Suppress(';')
	)

	def __init__(self, type, name, array=False, array_size=None):
		self.name = name
		self.type = type
		self.array = array
		self.array_size = array_size

	def isPOD(self):
		if self.array and not self.array_size:
			return False

		if self.type in BUILTIN_TYPES:
			return True

		return self.type.isPOD()

	def size(self):
		c = ""
		if self.array and self.array_size:
			c = "(" + self.array_size + ") * "
		
		if self.type in BUILTIN_TYPES:
			return c + str(BUILTIN_TYPES[self.type])

		return c + "(" + self.type.podSize() + ")"

	def __str__(self):
		s = self.type + " " + self.name
		if self.array:
			s += "[]"
		return s

	def definition(self, last=False):
		if self.array:
			if self.array_size:
				# Known array size
				return str(self.type) + " " + self.name + "[" + self.array_size + "];"
			else:
				# Dynamic array
				last = str(last).lower()
				return "uc::List< uc::IOInstance<IO, %s>, %s > %s;" % (
					last, self.type, self.name
				)
		else:
			return str(self.type) + " " + self.name + ";"

	def resolveType(self, types):
		if self.type in BUILTIN_TYPES:
			return

		try:
			self.type = types[self.type]
		except KeyError:
			raise UnknownTypeError(self.type)

	@classmethod
	def parse(cls, parse_result):
		array = False
		array_size = None

		if parse_result.array:
			array = True
			array_size = parse_result.array.size

		return cls(parse_result.type, parse_result.name, array, array_size)
registerParseAction(Member)

class Custom:
	content = Forward()
	content << (
		  (CharsNotIn('{}'))
		| (Literal('{') + ZeroOrMore(content) + Literal('}'))
	)

	grammar = (
		Suppress('custom')
		+ Suppress('{')
		+ ZeroOrMore(content)
		+ Suppress('}')
	)

	def __init__(self, content):
		self.content = content

	def __str__(self):
		return ' '.join(self.content)

	@classmethod
	def parse(cls, parse_result):
		return cls(parse_result)
registerParseAction(Custom)

class Struct:
	grammar = (
		  (Literal('struct') | Literal('msg'))('type')
		+ Identifier.grammar("name")
		+ Suppress('{')
		+ ZeroOrMore(Enum.grammar)("enums")
		+ ZeroOrMore(Member.grammar)("members")
		+ Suppress('}')
		+ Suppress(';')
	)

	def __init__(self, type, name, members, enums):
		self.type = type
		self.name = name
		self.members = list(members)
		self.enums = list(enums)

	def __str__(self):
		return self.name

	def setMsgID(self, id):
		self.msgID = id

	def definition(self):
		code = [
			'struct %s' % self.name,
			'{',
			'\tenum',
			'\t{',
			'\t\tIS_POD = %d,' % self.isPOD(),
			'\t\tPOD_SIZE = %s,' % self.podSize(),
			'\t};',
			'',
		]

		if self.type == 'msg':
			code += [
				'\tenum',
				'\t{',
				'\t\tMSG_CODE = %d' % self.msgID,
				'\t};',
				'',
			]

		if self.enums :
			code += [ m.definition(1) for m in self.enums ]

		if self.podMembers:
			code += [
				'\tstruct',
				'\t{',
				'\n'.join(['\t\t' + m.definition() for m in self.podMembers]),
				'\t} __attribute__((packed));',
			]

		for i, m in enumerate(self.nonPODMembers):
			last = i == len(self.nonPODMembers)-1
			code += [
				'\t' + m.definition(last)
			]

		code += [
			'',
			self.def_serialize(),
			self.def_deserialize(),
		]

		if self.isPOD():
			code.append('} __attribute__((packed));')
		else:
			code.append('};')

		return '\n'.join(code)

	def podSize(self):
		podMembers = [ m for m in self.members if m.isPOD() ]
		if len(self.podMembers) == 0:
			return "0"
		else:
			return ' + '.join([ "(" + m.size() + ")" for m in podMembers ])

	def isPOD(self):
		for m in self.members:
			if not m.isPOD():
				return False
		return True

	def def_serialize(self):
		code = [
			'inline bool serialize(typename IO::Handler* output) const',
			'{',
		]

		if self.podSize() > 0:
			code += [
				'\tRETURN_IF_ERROR(output->write(this, %s));' % self.podSize(),
			]

		code += ['\tRETURN_IF_ERROR(%s.serialize(output));' % m.name for m in self.nonPODMembers]

		code += [
			'\treturn true;',
			'}',
		]
		
		return ''.join([ '\t' + i + '\n' for i in code])

	def def_deserialize(self):
		code = [
			'inline bool deserialize(typename IO::Reader* input)',
			'{',
		]

		if self.podSize() > 0:
			code += [
				'\tRETURN_IF_ERROR(input->read(this, %s));' % self.podSize(),
			]

		for i, m in enumerate(self.nonPODMembers):
			last = 'false'
			if i == len(self.nonPODMembers)-1:
				last = 'true'

			code.append('\tRETURN_IF_ERROR(%s.deserialize(input));' % m.name)

		code += [
			'\treturn true;',
			'}',
		]

		return ''.join([ '\t' + i + '\n' for i in code])

	def resolveTypes(self, types):
		self.podMembers = []
		self.nonPODMembers = []
	
		for m in self.members:
			m.resolveType(types)

			if m.isPOD():
				self.podMembers.append(m)
			else:
				self.nonPODMembers.append(m)

	@classmethod
	def parse(cls, parse_result):
		return cls(parse_result.type, parse_result.name, parse_result.members, parse_result.enums)
registerParseAction(Struct)


class Grammar:
	def __init__(self):
		self.document = ZeroOrMore(Struct.grammar | Custom.grammar | Enum.grammar)

class Parser:
	def __init__(self, grammar):
		self.grammar = grammar

	def parse(self, string):
		# Strip comments
		content = ""
		for line in string.split('\n'):
			pos = line.find('//')
			if pos < 0:
				content += line + '\n'
			else:
				content += line[0:pos] + '\n'

		ret = self.grammar.document.parseString(content, True)

		types = {}

		enums = [ s for s in ret if isinstance(s, Enum) ]
		structs = [ s for s in ret if isinstance(s, Struct) ]
		custom_areas = [ s for s in ret if isinstance(s, Custom) ]

		for struct in structs:
			if struct.name in types:
				raise RuntimeError, "Struct '%s' is defined multiple times" % struct.name

			types[struct.name] = struct

		print '#include <stdint.h>'
		print '#include <stdlib.h>'
		print '#include <libucomm/list.h>'
		print

		print '// Start custom area'
		for custom in custom_areas:
			print custom
		print '// End custom area'
		print

		print 'template<class IO>'
		print 'class Proto'
		print '{'
		print 'public:'

		msg_counter = 0

		print
		for enum in enums:
			print enum.definition() + "\n"

		for struct in structs:
			struct.resolveTypes(types)

			if struct.type == 'msg':
				struct.setMsgID(msg_counter)
				msg_counter += 1

			print struct.definition() + "\n"

		print '};'

class Generator:
	def __init__(self):
		pass

if __name__ == "__main__":
	import sys
	grammar = Grammar()
	parser = Parser(grammar)
	parser.parse(open(sys.argv[1]).read())
