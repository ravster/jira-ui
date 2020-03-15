set cred_file [open creds]
set creds [gets $cred_file]
set subdomain [gets $cred_file]
close $cred_file

# Yes, I'm using globals.  Deal with it.
set prompt ""
set issue ""

# Usage: issue abc-234
proc issue {str} {
    global ::issue ::prompt
    set issue [exec curl -sSL -u $::creds "https://$::subdomain.atlassian.net/rest/api/latest/issue/$str"]
    set summary [exec jq {.fields.summary} << $issue]
    set prompt "$str: $summary"
}

proc description {} {
    puts [exec jq -r {[.fields.description] | @tsv} << $::issue]
}
proc d {} {
    description
}

while {1} {
    puts "$prompt >"
    set input [gets stdin]
    eval $input
}
