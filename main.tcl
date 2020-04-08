package require json
package require json::write

set cred_file [open creds]
set creds [gets $cred_file]
set subdomain [gets $cred_file]
close $cred_file

# Yes, I'm using globals.  Deal with it.
set prompt ""
set issue ""
set issue_id ""

# Go to issue
# Usage: g abc-234
proc g {str} {
    global ::issue ::prompt ::issue_id
    set issue [exec curl -sSL -u $::creds "https://$::subdomain.atlassian.net/rest/api/latest/issue/$str"]
    set summary [exec jq {.fields.summary} << $issue]
    set issue_id $str
    set prompt "$issue_id: $summary"
}

# Refresh
proc r {} {
    g $::issue_id
}

proc description {} {
    set d1 [exec jq -r {[.fields.description] | @tsv} << $::issue]
    regsub -all {\\r\\n} $d1 "\n" d2
    regsub -all {\\n} $d2 "\n" d3
    puts $d3
}
proc d {} {
    description
}

# Print comments for current issue
proc c {} {
    global ::issue
    set comments [exec jq -r {.fields.comment.comments[] | [.author.displayName, .body] | @tsv} << $issue ]
    set lines [split $comments "\n"]
    foreach line $lines {
	puts $line
	puts "-----"
    }
}

proc mc {} {
    set a1 [gets stdin]
    # TODO JSON-escape any double-quotes within the input.
    set body "{\"body\":\"$a1\"}"
    set fh [open temp w]
    puts $fh $body
    close $fh
    exec -ignorestderr curl -sSL -u $::creds -H content-type:application/json -d @temp "https://$::subdomain.atlassian.net/rest/api/2/issue/$::issue_id/comment"
}

# Assign issue to myself
proc i {} {
    set fh [ open temp w ]
    puts $fh {{"name":"Ravi Desai"}}
    close $fh
    exec curl -X PUT -sSL -u $::creds -H content-type:application/json -d @temp "https://$::subdomain.atlassian.net/rest/api/2/issue/$::issue_id/assignee"
}

proc unassign {} {
    set fh [ open temp w ]
    puts $fh {{"name": null}}
    close $fh
    exec curl -X PUT -sSL -u $::creds -H content-type:application/json -d @temp "https://$::subdomain.atlassian.net/rest/api/2/issue/$::issue_id/assignee"
}

proc s {str} {
    puts "Searching for $str in text"
    set body [::json::write object jql "\"text ~ '$str' and status = Done\""]
    puts $body
    set fh [open temp w]
    puts $fh $body
    close $fh
    set results [exec curl -sSL -H "Content-Type:application/json" -d @temp -u $::creds "https://$::subdomain.atlassian.net/rest/api/2/search"]
    set r1 [::json::json2dict $results]
    set issues [dict get $r1 issues]
    set i1 0
    foreach i $issues {
    	set key [dict get $i key]
    	set summary [dict get $i fields summary]
    	puts "$key : $summary"
    	incr i1
    }
    puts "Found $i1 results"
}

proc h {} {
    puts "h - print this message"
    puts "g - go to issue"
    puts "r - refresh issue"
    puts "d - description"
    puts "c - list comments"
    puts "mc - make comment"
    puts "s - search text in non-done issues"
    puts "unassign"
}

while {1} {
    puts "$prompt >"
    set input [gets stdin]
    eval $input
}
