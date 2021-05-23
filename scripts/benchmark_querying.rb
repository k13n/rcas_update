require "optparse"

def outfile(bulkload_percent, method)
  percent_str = "%03d" % (bulkload_percent * 100)
  "#{OUTPUT_FOLDER}/bulk#{percent_str}_#{method}.out.txt"
end

def perffile_raw(bulkload_percent, method)
  percent_str = "%03d" % (bulkload_percent * 100)
  "#{OUTPUT_FOLDER}/perf_#{percent_str}_#{method}.data"
end

def perffile_report(bulkload_percent, method)
  percent_str = "%03d" % (bulkload_percent * 100)
  "#{OUTPUT_FOLDER}/perfreport_#{percent_str}_#{method}.txt"
end


def execute_benchmark(bulkload_percent, method)
  system "./release/benchmark_cache_misses \
    --input_filename='#{INPUT_FILE}' \
    --bulkload_percent=#{bulkload_percent} \
    --insert_method=#{method} \
    --perf_datafile=#{perffile_raw(bulkload_percent, method)} \
    > #{outfile(bulkload_percent, method)} 2>&1"
end


def execute_perf_report(bulkload_percent, method)
  system "perf report \
      --show-nr-samples \
      --show-total-period \
      -g 'graph,0.5,caller,period' \
      --children \
      --fields=+period \
      --field-separator=, \
      --demangle \
      --input=#{perffile_raw(bulkload_percent, method)} \
      > #{perffile_report(bulkload_percent, method)}"
end


def analyze_queryperformance(bulkload_percent, method)
  return unless File.exists? outfile(bulkload_percent, method)
  content = File.readlines(outfile(bulkload_percent, method)).reverse
  runtime_ms_str = content.select{ |line| line =~ /runtime_ms:.*/ }.first.chomp
  read_nodes_str = content.select{ |line| line =~ /read_nodes_:.*/ }.first.chomp
  runtime_ms = runtime_ms_str.match(/.* (.+)/)[1]
  read_nodes = read_nodes_str.match(/.* (.+)/)[1]
  puts [
    bulkload_percent,
    method,
    runtime_ms,
    read_nodes
  ].join(";")
end


def analyze_perf(bulkload_percent, method)
  content = File.readlines(perffile_report(bulkload_percent, method))
  measurements = []
  content.each_with_index do |line, index|
    if line =~ /\[\.\] cas::Query<long>::Execute/
      measurements << content[index+2].match(/--(\d+)--/)[1].to_i
    end
  end
  task_clock          = measurements[0]
  instructions        = measurements[1]
  cycles              = measurements[2]
  cache_references    = measurements[3]
  cache_misses        = measurements[4]
  branch_instructions = measurements[5]
  branch_misses       = measurements[6]
  instructions_per_cycle = instructions / cycles.to_f
  cache_miss_ratio = cache_misses / cache_references.to_f
  branch_miss_ratio = branch_misses / branch_instructions.to_f
  puts [
    bulkload_percent,
    method,
    task_clock,
    instructions,
    cycles,
    cache_references,
    cache_misses,
    branch_instructions,
    branch_misses,
    instructions_per_cycle,
    cache_miss_ratio,
    cache_miss_ratio * 100,
    branch_miss_ratio,
  ].join(";")
end


def execute
  # make sure the code is compiled
  Dir.chdir "#{__dir__}/../release"
  system "make"
  Dir.chdir "#{__dir__}/.."

  bulkload_percents = [0.6, 0.7, 0.8, 0.9, 1.0]
  methods = [0, 1, 2, 3]

  bulkload_percents.each do |bulkload_percent|
    methods.each do |method|
      execute_benchmark(bulkload_percent, method)
      execute_perf_report(bulkload_percent, method)
    end
  end

  puts "query performance"
  puts "bulkload_percent;method;runtime_ms;read_nodes"
  methods.each do |method|
    bulkload_percents.each do |bulkload_percent|
      analyze_queryperformance(bulkload_percent, method)
    end
  end

  puts "\n\ncache misses"
  puts [
    "bulkload_percent",
    "method",
    "task_clock",
    "instructions",
    "cycles",
    "cache_references",
    "cache_misses",
    "branch_instructions",
    "branch_misses",
    "instructions_per_cycle",
    "cache_miss_ratio",
    "cache_miss_percent",
    "branch_miss_ratio",
  ].join(";")
  methods.each do |method|
    bulkload_percents.each do |bulkload_percent|
      analyze_perf(bulkload_percent, method)
    end
  end
end


options = {}
OptionParser.new do |parser|
  parser.on("-i", "--input_file INFILE", "Dataset stored in INFILE") do |v|
    options[:input_file] = v
  end
  parser.on("-o", "--output_folder OUTFOLDER", "Output stored in OUTFOLDER") do |v|
    options[:output_folder] = v
  end
end.parse!

if options[:input_file].nil?
  puts "input file needed"
  exit
else
  INPUT_FILE = options[:input_file]
end

if options[:output_folder].nil?
  puts "output folder needed"
  exit
else
  OUTPUT_FOLDER = options[:output_folder]
end

execute
