<?php

$hostname = "localhost";
$username = "username";
$password = "password";
$database = "sensor_db";

$conn = mysqli_connect($hostname, $username, $password, $database);

if(!$conn){
    die("Connection failed: " . mysqli_connect_error());
}

echo "Database connection is OK ";


if(isset($_POST["temperature"]) && isset($_POST["spo2"]) && isset($_POST["bpm"]) ) {

	$t = $_POST["temperature"];
	$s = $_POST["spo2"];
    $b = $_POST["bpm"];

	$sql = "INSERT INTO dht11 (temperature, spo2, bpm) VALUES (".$t.", ".$s.", ".$b.")"; 

	if (mysqli_query($conn, $sql)) { 
		echo "\nNew record created successfully"; 
	} else { 
		echo "Error: " . $sql . "<br>" . mysqli_error($conn); 
	}
}

?>