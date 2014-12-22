#!/usr/bin/perl -w

use strict; 
use Data::Dumper;

# ITEMSMAP 
# defines the perl name of items defined in nfdump structures 
# the structure contains the ID, perl name and required extensions.
# Some of items can be mapped to multiple extensions (for example 
# output counters can be mapped to EX_OUT_BYTES_4 or EX_OUT_BYTES_8. 
# In that case the longer option will be used. This option 
# have to be written here as the first item. 

my @ITEMS_MAP = (
  {	name => 'first',		
	rcode => 'AV_STORE_NV(res, rec->first)',	wcode => 'rec->first = SvUV(sv)',  
	extensions => [ 'COMMON_BLOCK_ID' ] }
);


# The file where map extensions are described 
# this file should contain text descriptions for 
# extensions. 
my $MAP_DESCR_FILE = "nfdump/bin/nfx.c";


# the file where master_record structure is defined 
my $MASTER_RECORD_FILE = "nfdump/bin/nffile.h";

# path to libnf C and H source files
my $LIBNF_C_FILE = "src/fields.c";


# The perl structure parsed from 
# extension_descriptor[] from MAP_DESCR_FILE. 
# The structure is filled byt get_map_descr subrutine
my @MAP_DESCR;

# master record structure contains the name of item 
# in master record and type 
my @MASTER_RECORD;

# hash containing the content of the C file functions 
# enclosed between  comments 
# /* TAG for check_items_map.pl: function_name */
# and ^}$ . 
my %LIBNF_C_FUNC;


sub get_map_descr() {

	if ( ! -f $MAP_DESCR_FILE ) {
		die "Can't find file $MAP_DESCR_FILE";
	}

	open F1, "< $MAP_DESCR_FILE";

	while (<F1>) {
		
		# { COMMON_BLOCK_ID,      0,   0, 1,   "Required extension: Common record"},
		if (/\s*{\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*("(.+)"|(NULL))\s*}/) { 
			my ($id, $size, $user_index, $enabled, $description) = ($1, $2, $3, $4, $5);

			$description =~ s/"//g;
			$description =~ s/NULL//g;
			last if ($id eq "0" );	# last entry

			push(@MAP_DESCR, { "id" => $id, "size" => $size, "user_index" => $user_index,
								"enabled" => $enabled, "description" => $description } );
		}
	}
}


sub get_master_record() {

	if ( ! -f $MASTER_RECORD_FILE ) {
		die "Can't find file $MASTER_RECORD_FILE";
	}

	open F1, "< $MASTER_RECORD_FILE";
	my $in_master = 0;

	my $order = 0;
	while (<F1>) {
		chomp;

		if (/typedef struct master_record_s {/) {
			$in_master = 1;
			next;

		}

		if (/} master_record_t;/) {
			$in_master = 0;
			last;
		}

		next if (!$in_master);
	
		# here we are in the master_record structure 	
	
		# remove macros, comments and wmpty lisnes 
		s/\/\/.*//;
		next if (/#/);
		next if (/^\s*$/);
		# uint32_t	srcas;
#		printf "$_\n";
		if (/\s+(\w+_t|char)\s+(\w+)([\[\]\d+]*);/) {
			my ($type, $name, $arr) = ($1, $2, $3);

			if (defined($arr)) {
				$type .= $arr;
			}

		#	printf "$type | $name | $arr \n";

			# ignore items like fill, any 
			next if ($name =~ /fill/ || $name =~ /^any$/ || $name =~ /^ext_map$/);

			push(@MASTER_RECORD, { 'name' => $name, 'type' => $type });
		}
	}
}

sub get_func_content() {

	if ( ! -f $LIBNF_C_FILE ) {
		die "Can't find file $LIBNF_C_FILE";
	}

	open F1, "< $LIBNF_C_FILE";
	my $funcname = undef;

	my $order = 0;
	while (<F1>) {
#		chomp;

		# /* TAG for check_items_map.pl: function_name */
		# and ^}$ . 
		#if (/TAG for check_items_map.pl: (\w+)/) {
		if (/static int inline lnf_field_(f[gs]et)_/) {
			$funcname = $1;
#			$LIBNF_C_FUNC{$funcname} = "";
			next;
		}
		if (/^}$/) {
			$funcname = undef;
		}

		if (defined($funcname)) {
			if (defined($LIBNF_C_FUNC{$funcname})) {
				$LIBNF_C_FUNC{$funcname} .= $_;
			} else {
				$LIBNF_C_FUNC{$funcname} = $_;
			}
		}
	}
}

# recurns the number of apperance $substring in $string
sub str_count($$) {
	my ($string, $substring) = @_;

	$string =~ s/\n/ /g;
#	print "XXX $substring XXXX $string XXX  \n";
	my $count = 0;
	while ($string =~ /$substring/g) { $count++ }
	return $count;
}

# load map description from nfdump source files 
get_map_descr();
#print Dumper(\@MAP_DESCR);

get_master_record();
#print Dumper(\@MASTER_RECORD);


get_func_content();
#print Dumper(\%LIBNF_C_FUNC);


my $invalid = 0;
# checking extensions
printf "Missing definitionin libnf for following extension:\n";
printf "EXT_NAME              | read | write | description\n";
printf "----------------------+------+-------+------------\n";
foreach (@MAP_DESCR) {

	# skip "Required extension" and extension with no description 
	next if ($_->{'description'} =~ /^Required extension:/);
	next if ($_->{'description'} =~ /^$/);
	next if ($_->{'description'} =~ /Compat NEL/);

	my $read = str_count($LIBNF_C_FUNC{'fget'}, $_->{'id'}); 
	my $write = str_count($LIBNF_C_FUNC{'fset'}, $_->{'id'}); 

	next if ($read > 0 && $write > 0);

	printf "%-20s  |    %d |     %d | %s \n", $_->{'id'}, $read, $write, $_->{'description'};
	$invalid = 1;
}

if (!$invalid) {
	printf "All extensions are defined for both read and write function.\n";
}


# checking fields from master record
printf "\nMissing definitionin libnf for following fields:\n";
printf "FIELD NAME            | read | write | data type\n";
printf "----------------------+------+-------+------------\n";
foreach (@MASTER_RECORD) {

	# skip type and size records
	next if ($_->{'name'} =~ /type|size/); 
	next if ($_->{'name'} =~ /u\d{2}_\d/);
	next if ($_->{'name'} =~ /exporter_sysid/); 
	next if ($_->{'name'} =~ /nat_flags/); 

	my $read = str_count($LIBNF_C_FUNC{'fget'}, $_->{'name'}); 
	my $write = str_count($LIBNF_C_FUNC{'fset'}, $_->{'name'}); 

	next if ($read > 0 && $write > 0);

	printf "%-20s  |    %d |     %d | %s \n", $_->{'name'}, $read, $write, $_->{'type'};
	$invalid = 1;
}

if (!$invalid) {
	printf "All fields are processed in both read and write function.\n";
}

exit $invalid;
