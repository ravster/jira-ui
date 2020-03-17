set cred_file [open creds]
set creds [gets $cred_file]
set subdomain [gets $cred_file]
close $cred_file

# Yes, I'm using globals.  Deal with it.
set prompt ""
set issue ""

# Set issue
# Usage: si abc-234
proc si {str} {
    global ::issue ::prompt
    set issue [exec curl -sSL -u $::creds "https://$::subdomain.atlassian.net/rest/api/latest/issue/$str"]
    set summary [exec jq {.fields.summary} << $issue]
    set prompt "$str: $summary"
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

while {1} {
    puts "$prompt >"
    set input [gets stdin]
    eval $input
}
