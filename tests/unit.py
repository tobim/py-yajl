#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest

from cStringIO import StringIO

import yajl

class DecoderBase(unittest.TestCase):
    def decode(self, json):
        return yajl.loads(json)

    def assertDecodesTo(self, json, value):
        rc = self.decode(json)
        assert rc == value, ('Failed to decode JSON correctly',
                json, value, rc)
        return True

class BasicJSONDecodeTests(DecoderBase):
    def test_TrueBool(self):
        self.assertDecodesTo('true', True)

    def test_FalseBool(self):
        self.assertDecodesTo('false', False)

    def test_Null(self):
        self.assertDecodesTo('null', None)

    def test_List(self):
        self.assertDecodesTo('[1,2]', [1, 2])

    def test_ListOfFloats(self):
        self.assertDecodesTo('[3.14, 2.718]', [3.14, 2.718])

    def test_Dict(self):
        self.assertDecodesTo('{"key" : "pair"}', {'key' : 'pair'})

    def test_ListInDict(self):
        self.assertDecodesTo('''
            {"key" : [1, 2, 3]}
        ''', {'key' : [1, 2, 3]})

    def test_DictInDict(self):
        self.assertDecodesTo('''
            {"key" : {"subkey" : true}}''',
                {'key' : {'subkey' : True}})

    def test_NestedDictAndList(self):
        self.assertDecodesTo('''
            {"key" : {"subkey" : [1, 2, 3]}}''',
                {'key' : {'subkey' : [1,2,3]}})


class EncoderBase(unittest.TestCase):
    def encode(self, value):
        return yajl.dumps(value)

    def assertEncodesTo(self, value, json):
        rc = self.encode(value)
        assert rc == json, ('Failed to encode JSON correctly', locals())
        return True

class BasicJSONEncodeTests(EncoderBase):
    def test_TrueBool(self):
        self.assertEncodesTo(True, 'true')

    def test_FalseBool(self):
        self.assertEncodesTo(False, 'false')

    def test_Null(self):
        self.assertEncodesTo(None, 'null')

    def test_List(self):
        self.assertEncodesTo([1,2], '[1,2]')

    def test_Dict(self):
        self.assertEncodesTo({'key' : 'value'}, '{"key":"value"}')

    # Python 3 version
    #def test_UnicodeDict(self):
    #        self.assertEncodesTo({'foō' : 'bār'}, '{"foō":"bār"}')

    # Python 2 version
    #def test_UnicodeDict(self):
    #        self.assertEncodesTo({u'foō' : u'bār'}, '{"foō":"bār"}')

    def test_NestedDictAndList(self):
        self.assertEncodesTo({'key' : {'subkey' : [1,2,3]}},
            '{"key":{"subkey":[1,2,3]}}')
    def test_Tuple(self):
        self.assertEncodesTo((1,2), '[1,2]')
    def test_generator(self):
        def f():
            for i in range(10):
                yield i
        self.assertEncodesTo(f(), '[0,1,2,3,4,5,6,7,8,9]')

    def test_class(self):
        class Bad(object):
            pass
        self.assertRaises(TypeError, yajl.dumps, Bad)


class ErrorCasesTests(unittest.TestCase):

    def test_EmptyString(self):
        self.failUnlessRaises(ValueError, yajl.loads, '')

    def test_None(self):
        self.failUnlessRaises(ValueError, yajl.loads, None)


class StreamBlockingDecodingTests(unittest.TestCase):
    def setUp(self):
        self.stream = StringIO('{"foo":["one","two", ["three", "four"]]}')

    def test_no_object(self):
        self.failUnlessRaises(TypeError, yajl.load)

    def test_bad_object(self):
        self.failUnlessRaises(TypeError, yajl.load, 'this is no stream!')

    def test_simple_decode(self):
        obj = yajl.load(self.stream)
        self.assertEquals(obj, {'foo' : ['one', 'two', ['three', 'four']]})


class StreamEncodingTests(unittest.TestCase):
    def test_blocking_encode(self):
        obj = {'foo' : ['one', 'two', ['three', 'four']]}
        stream = StringIO()
        buffer = yajl.dump(obj, stream)
        self.assertEquals(stream.getvalue(), '{"foo":["one","two",["three","four"]]}')

class DumpsOptionsTests(unittest.TestCase):
    def test_indent_four(self):
        rc = yajl.dumps({'foo' : 'bar'}, indent=4)
        expected = '{\n    "foo": "bar"\n}\n'
        self.assertEquals(rc, expected)

    def test_indent_zero(self):
        rc = yajl.dumps({'foo' : 'bar'}, indent=0)
        expected = '{\n"foo": "bar"\n}\n'
        self.assertEquals(rc, expected)

    def test_indent_str(self):
        self.failUnlessRaises(TypeError, yajl.dumps, {'foo' : 'bar'}, indent='4')

    def test_negative_indent(self):
        ''' Negative `indent` should not result in pretty printing '''
        rc = yajl.dumps({'foo' : 'bar'}, indent=-1)
        self.assertEquals(rc, '{"foo":"bar"}')

class DumpOptionsTests(unittest.TestCase):
    stream = None
    def setUp(self):
        self.stream = StringIO()

    def test_indent_four(self):
        rc = yajl.dump({'foo' : 'bar'}, self.stream, indent=4)
        expected = '{\n    "foo": "bar"\n}\n'
        self.assertEquals(self.stream.getvalue(), expected)

    def test_indent_zero(self):
        rc = yajl.dump({'foo' : 'bar'}, self.stream, indent=0)
        expected = '{\n"foo": "bar"\n}\n'
        self.assertEquals(self.stream.getvalue(), expected)

    def test_indent_str(self):
        self.failUnlessRaises(TypeError, yajl.dump, {'foo' : 'bar'}, self.stream, indent='4')

    def test_negative_indent(self):
        ''' Negative `indent` should not result in pretty printing '''
        rc = yajl.dump({'foo' : 'bar'}, self.stream, indent=-1)
        self.assertEquals(self.stream.getvalue(), '{"foo":"bar"}')

class IssueSevenTest(unittest.TestCase):

    def test_DecodeLatin1(self):
        # TODO: could expose dont_validate option from yajl
        return
        obj = yajl.loads('"f\xe9in"')
        print(obj)

    def test_latin1(self):
        ''' Testing with latin-1 for http://github.com/rtyler/py-yajl/issues/#issue/7 '''
        IssueSevenTest_latin1_char = u'f\xe9in'
        char = IssueSevenTest_latin1_char

        # The `json` module uses "0123456789abcdef" for its code points
        # while the yajl library uses "0123456789ABCDEF", lower()'ing
        # to make sure the resulting strings match
        out = yajl.dumps(char).lower()
        self.assertEquals(out, '"f\\u00e9in"')

        out = yajl.dumps(out).lower()
        self.assertEquals(out, '"\\"f\\\\u00e9in\\""')

        out = yajl.loads(out)
        self.assertEquals(out, '"f\\u00e9in"')

        out = yajl.loads(out)
        self.assertEquals(out, char)

    def test_chinese(self):
        ''' Testing with simplified chinese for http://github.com/rtyler/py-yajl/issues/#issue/7 '''
        IssueSevenTest_chinese_char = u'\u65e9\u5b89, \u7238\u7238'
        char = IssueSevenTest_chinese_char

        out = yajl.dumps(char).lower()
        self.assertEquals(out, '"\\u65e9\\u5b89, \\u7238\\u7238"')

        out = yajl.dumps(out).lower()
        self.assertEquals(out, '"\\"\\\\u65e9\\\\u5b89, \\\\u7238\\\\u7238\\""')

        out = yajl.loads(out)
        self.assertEquals(out, '"\\u65e9\\u5b89, \\u7238\\u7238"')

        out = yajl.loads(out)
        self.assertEquals(out, char)


class IssueEightTest(unittest.TestCase):
    def runTest(self):
        ''' http://github.com/rtyler/py-yajl/issues#issue/8 '''
        encoded = yajl.dumps([(2,3,)])
        decoded = yajl.loads(encoded)
        self.assertEquals(len(decoded), 1)
        self.assertEquals(decoded[0][0], 2)
        self.assertEquals(decoded[0][1], 3)

class IssueNineTest(unittest.TestCase):
    def testListOfSets(self):
        ''' http://github.com/rtyler/py-yajl/issues#issue/9 '''
        # TODO: Should be fixed
        self.failUnlessRaises(TypeError, yajl.dumps, [set([2,3])])

    def testSets(self):
        ''' http://github.com/rtyler/py-yajl/issues#issue/9 '''
        # TODO: Should be fixed
        self.failUnlessRaises(TypeError, yajl.dumps, set([2,3]))


class IssueTenTest(unittest.TestCase):
    def testInt(self):
        ''' http://github.com/rtyler/py-yajl/issues#issue/10 '''
        data = {1 : 2}
        self.assertRaises(TypeError, yajl.dumps, data)

    def testFloat(self):
        ''' http://github.com/rtyler/py-yajl/issues#issue/10 '''
        data = {1.2 : 2}
        self.assertRaises(TypeError, yajl.dumps, data)

    def testLong(self):
        ''' http://github.com/rtyler/py-yajl/issues#issue/10 '''
        data = {long(1) : 2}
        self.assertRaises(TypeError, yajl.dumps, data)

class IssueTwelveTest(unittest.TestCase):
    def runTest(self):
        normal = {'a' : 'b', 'c' : 'd'}
        self.assertEquals(yajl.dumps(normal), '{"a":"b","c":"d"}')

        IssueTwelveTest_dict = {u'a' : u'b', u'c' : u'd'}
        self.assertEquals(yajl.dumps(IssueTwelveTest_dict), '{"a":"b","c":"d"}')


class IssueSixteenTest(unittest.TestCase):
    def runTest(self):
        dumpable = [11889582081]

        rc = yajl.dumps(dumpable)
        self.assertEquals(rc, '[11889582081]')
        rc = yajl.loads(rc)
        self.assertEquals(rc, dumpable)


class IssueTwentySevenTest(unittest.TestCase):
    "https://github.com/rtyler/py-yajl/issues/27"
    def runTest(self):
        u = u'[{"data":"Podstawow\u0105 opiek\u0119 zdrowotn\u0105"}]'
        self.assertEqual(
                yajl.dumps(yajl.loads(u)),
                '[{"data":"Podstawow\\u0105 opiek\\u0119 zdrowotn\\u0105"}]')


if __name__ == '__main__':
    verbosity = '-v' in sys.argv and 2 or 1
    runner = unittest.TextTestRunner(verbosity=verbosity)
    if 'xml' in sys.argv:
        import xmlrunner
        runner = xmlrunner.XMLTestRunner(filename='Yajl-Tests.xml')
        suites = unittest.findTestCases(sys.modules[__name__])
        results = runner.run(unittest.TestSuite(suites))
    else:
        unittest.main()

