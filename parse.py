
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

class Member:
	grammar = (
		  Identifier.grammar("type")
		+ Identifier.grammar("name")
		+ Optional(Literal('[]'))("array")
		+ Suppress(';')
	)

	def __init__(self, type, name, array=False):
		self.name = name
		self.type = type
		self.array = array

	def isPOD(self):
		if self.type in BUILTIN_TYPES:
			return True

		if self.array:
			return False

		return self.type.isPOD()

	def size(self):
		if self.type in BUILTIN_TYPES:
			return BUILTIN_TYPES[self.type]

		return self.type.podSize()

	def __str__(self):
		s = self.type + " " + self.name
		if self.array:
			s += "[]"
		return s

	def definition(self, last=False):
		if self.array:
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

		if parse_result.array:
			array = True

		return cls(parse_result.type, parse_result.name, array)
registerParseAction(Member)

class Struct:
	grammar = (
		  (Literal('struct') | Literal('msg'))('type')
		+ Identifier.grammar("name")
		+ Suppress('{')
		+ ZeroOrMore(Member.grammar)("members")
		+ Suppress('}')
		+ Suppress(';')
	)

	def __init__(self, type, name, members):
		self.type = type
		self.name = name
		self.members = list(members)

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
			'\t\tPOD_SIZE = %d,' % self.podSize(),
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
			'};',
		]

		return '\n'.join(code)

	def podSize(self):
		return sum([ m.size() for m in self.members if m.isPOD()])

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
				'\tRETURN_IF_ERROR(output->write(this, %d));' % self.podSize(),
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
				'\tRETURN_IF_ERROR(input->read(this, %d));' % self.podSize(),
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
		return cls(parse_result.type, parse_result.name, parse_result.members)
registerParseAction(Struct)


class Grammar:
	def __init__(self):
		self.document = ZeroOrMore(Struct.grammar)

class Parser:
	def __init__(self, grammar):
		self.grammar = grammar

	def parse(self, f):
		ret = self.grammar.document.parseString(f, True)

		types = {}

		for struct in ret:
			if struct.name in types:
				raise RuntimeError, "Struct '%s' is defined multiple times" % struct.name

			types[struct.name] = struct

		print '#include <stdint.h>'
		print '#include <stdlib.h>'
		print '#include <libucomm/list.h>'
		print
		print 'template<class IO>'
		print 'class Proto'
		print '{'
		print 'public:'

		msg_counter = 0

		for struct in ret:
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
