<?php

require_once("common.php");

$passed = 0;
$failed = 0;

function test_get_dn_by_name($oc,$od,$expected_name)
{
    global $passed, $failed;
    $name = get_device_name_by_oc_od($oc,$od);
    if ($name == $expected_name)
        $passed += 1;
    else
    {
        $failed += 1;
        echo "FAILED test_get_dn_by_name($od,$od,$expected_name)\n";
    }


}

test_get_dn_by_name( 'hspr', 'H101', "Treo 600" );
test_get_dn_by_name( 'sony', 'glps', "PEG-SJ22");
test_get_dn_by_name( 'qcom', 'qc20', "QCP 6035");

echo "PASSED: $passed\n";
echo "FAILED: $failed\n";
?>
