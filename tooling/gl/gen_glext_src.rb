#!/usr/bin/env ruby

require 'nokogiri'
require 'optparse'

REQUESTED_FNS_PATH = "#{__dir__}/../../res/gen/gl_fns.txt"
GL_REGISTRY_PATH = "#{__dir__}/../../external/OpenGL-Registry/xml/gl.xml"

GL_PROC_TYPE = "GLFWglproc"
GL_LOOKUP_FN = "glfwGetProcAddress"
ADDR_ARR = 'fn_addrs'

SRC_HEADER =
'/* Auto-generated file; do not modify! */

#define GL_GLEXT_PROTOTYPES

#include "GL/gl.h"
#include "GL/glext.h"
#include "GLFW/glfw3.h"
'

EXTERN_C_START =
'#ifdef __cplusplus
extern "C" {
#endif
'

EXTERN_C_END =
'#ifdef __cplusplus
}
#endif
'

INIT_CODE_GLOBAL =
"static #{GL_PROC_TYPE} #{ADDR_ARR}[%d];

namespace glext {
    void init_opengl_extensions() {
%s
    }
}

"

class FnParam
    def initialize(name, type)
        @name = name
        @type = type
    end
    attr_reader :name
    attr_reader :type

    def to_s()
        return "#{@type} #{@name}"
    end
end

class GLFunction
    def initialize(name, ret_type, params)
        @name = name
        @ret_type = ret_type
        @params = params
    end
    attr_reader :name
    attr_reader :ret_type
    attr_reader :params

    def to_s()
        fn_def = "#{@ret_type} #{@name}(#{params.join ', '})"
        return fn_def
    end
end

def params_to_str(params)
    params.join(', ')
end

def parse_args()
    options = {}
    OptionParser.new do |opts|
        opts.banner = "Usage: gen_glext_src.rb [--per-context] <output>"

        opts.on("-c", "--per-context", "Load functions per-context rather than globally") do |pc|
            options[:per_context] = true
        end
    end.parse!

    options[:output] = ARGV.pop
    raise "Output path is required" unless options[:output]

    options
end

def get_requested_fn_names()
    File.readlines(REQUESTED_FNS_PATH).each do |line|
        line.strip!
    end
end

def parse_param_type(raw)
    raw.gsub(/<name>.*<\/name>/, '').gsub(/<\/?ptype>/, '')
end

def load_fn_defs(fn_names)
    fns = []

    spec = File.open(GL_REGISTRY_PATH) { |f| Nokogiri::XML(f) }
    spec.xpath("//registry//commands//command").each do |cmd_root|
        cmd_name = cmd_root.xpath('.//proto//name')

        next unless fn_names.include? cmd_name.text

        name = cmd_name.text.strip

        ret = parse_param_type cmd_root.xpath('.//proto').inner_html
        ret.strip!

        params = []

        cmd_root.xpath('.//param').each do |cmd_param|
            param_name = cmd_param.xpath('.//name').text
            param_type = parse_param_type cmd_param.inner_html
            params << FnParam.new(param_name.strip, param_type.strip)
        end

        fns << GLFunction.new(name, ret, params)
    end

    fns
end

def write_src_file(out_file, fns, per_context)
    out_file << SRC_HEADER
    out_file << "\n"

    init_code = ''
    decl_code = ''

    fns.each_with_index do |fn, i|
        init_code << "        #{ADDR_ARR}[#{i}] = #{GL_LOOKUP_FN}(\"#{fn.name}\");\n"

        typed_params = fn.params.join ', '
        untyped_params = fn.params.map { |p| p.name }.join ', '
        decl_code << "\nAPIENTRY #{fn} {\n"
        decl_code << "    return ((#{fn.ret_type} (*)(#{typed_params})) #{ADDR_ARR}[#{i}])(#{untyped_params});\n"
        decl_code << "}\n"
    end

    init_code.delete_suffix! "\n"

    out_file << INIT_CODE_GLOBAL % [fns.size, init_code]

    out_file << EXTERN_C_START
    out_file << decl_code
    out_file << EXTERN_C_END
end

args = parse_args

per_context = args[:per_context]
out_path = args[:output]

fns = load_fn_defs(get_requested_fn_names())

out_file = File.open(out_path, 'w')

write_src_file(out_file, fns, per_context)
