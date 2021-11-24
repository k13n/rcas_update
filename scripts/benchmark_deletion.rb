require "optparse"

def execute
  # make sure the code is compiled
  Dir.chdir "#{__dir__}/../release"
  system "make"
  Dir.chdir "#{__dir__}/.."

  system "./release/benchmark_deletion \
    --input_filename='#{INPUT_FILE}' \
    --bulkload_percent=0.6 \
    --insert_method=0 \
    > #{OUTPUT_FOLDER}/main_sr.txt 2>&1"

  system "./release/benchmark_deletion \
    --input_filename='#{INPUT_FILE}' \
    --bulkload_percent=0.6 \
    --insert_method=1 \
    > #{OUTPUT_FOLDER}/main_lr.txt 2>&1"

  system "./release/benchmark_deletion \
    --input_filename='#{INPUT_FILE}' \
    --bulkload_percent=0.6 \
    --insert_method=2 \
    > #{OUTPUT_FOLDER}/mainaux_lr.txt 2>&1"

  system "./release/benchmark_deletion \
    --input_filename='#{INPUT_FILE}' \
    --bulkload_percent=0.6 \
    --insert_method=3 \
    > #{OUTPUT_FOLDER}/mainaux_sr.txt 2>&1"
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
