require 'net/http'
require 'json'

File.open("creds", "r") do |f|
  $basic_auth = f.gets.strip
  $subdomain = f.gets.strip
end

def goto_issue(id)
  uri = URI("https://#{$subdomain}.atlassian.net/rest/api/latest/issue/#{id}")
  req = Net::HTTP::Get.new(uri)
  req.basic_auth(*$basic_auth.split(":"))
  res = Net::HTTP.start(uri.hostname, uri.port, {use_ssl: true}) {|http|
    http.request(req)
  }

  $current_issue = JSON.parse(res.body, symbolize_names: true)
end

def description
  puts $current_issue.dig(:fields, :description)
end

def summary
  puts $current_issue.dig(:fields, :summary)
end

def comments
  $current_issue.dig(:fields, :comment, :comments).each do |comment|
    author = comment.dig(:author, :displayName)
    body = comment[:body]

    puts "#{author}: #{body}"
  end
end

def eval_command(input)
  command = input.strip!
  case
  when command.start_with?("g ")
    goto_issue(command.split(" ")[1])
  when command.start_with?("d")
    description
  when command.start_with?("s")
    summary
  when command.start_with?("c")
    comments
  else
    puts "Didn't understand that command.  RTFM."
  end
end

def read_command()
  print "> "
  gets
end

begin
  eval_command(read_command)
end while true
