<?php
use Aws\S3\S3Client;

function s3_put_file($bucket, $file_local, $file_remote){
    $retry = 5;
    $ret = array(
            'success' => false,
            'message' => "",
        );

    if (! is_file($file_local)){
        $ret['message'] = "S3: Local file '$file_local' is not present for uploading to s3 bucket '$bucket' as file '$file_remote'";
        return $ret;
    }

    $s3_result = s3_set_env();
    if (! $s3_result['s3']){
        $ret['message'] = "S3: Not able to create s3 client for uploading the file '$file_local' to s3 bucket '$bucket' as file '$file_remote'\n".$s3_result['e'];
        return $ret;
    }
    $s3 = $s3_result['s3'];

    for ($i = 1 ; $i <= $retry ; $i++ ){
        try {
            $s3->putObject([
                'Bucket' => $bucket,
                'Key'    => $file_remote,
                'Body'   => fopen($file_local, 'r'),
                'ACL'    => 'private',
            ]);
            $ret['success']=true;
            return $ret;
        } 
        catch (Exception $e) {
            if ($i == $retry){
                $ret['message'] = "S3: There was an error uploading the file '$file_local' to s3 bucket '$bucket' as file '$file_remote'.\n".$e->getMessage();
                return $ret;
            }
            usleep(500000);
            continue;
        }
        break;
    }
}

function s3_get_file($bucket, $file_remote, $type, $ext = ''){
    $retry = 5;
    $tmp_dir = '/var/dana';

    $ret = array(
            'data' => null,
            'success' => false,
            'message' => "",
        );

    if ($type == 'file'){
        if (! is_dir($tmp_dir)){
            if (!mkdir($tmp_dir, 0755, true)) {
                $ret['message'] = "S3: temp dir '$tmp_dir' not present and system is upable to create the same, hence not able to write remote file '$file_remote' to temp dir.";
                return $ret;
            }
        }
        if (! is_writable($tmp_dir)){
            $ret['message'] = "S3: temp dir $tmp_dir do not have write permission hence not able to write remote file '$file_remote' to temp dir.";
            return $ret;
        }
    }

    $s3_result = s3_set_env();
    if (! $s3_result['s3']){
        $ret['message'] = "S3: Not able to create s3 client for downloading the file '$file' from s3 bucket '$bucket'.\n".$s3_result['e'];
        return $ret;
    }
    $s3 = $s3_result['s3'];

    for ($i = 1 ; $i <= $retry ; $i++ ){

        try {
            $res = $s3->getObject(['Bucket' => $bucket, 'Key' => $file_remote]);
            $data = $res['Body']->getContents();

            if ($type == 'file'){
                $tmp_file = tempnam($tmp_dir, 'fs3_');
                if ($ext)
                    $tmp_file .= '.'.$ext;
                if (! file_put_contents($tmp_file, $data )){
                    if ($i == $retry){
                        $ret['message'] = "S3: Unable to write remote file '$file_remote' as local file '$tmp_file'.";
                        return $ret;
                    }
                    else
                        continue;
                }    

                $ret['success'] = true;
                $ret['data'] = $tmp_file;
                return $ret;
            }
            else {
                $ret['success'] = true;
                $ret['data'] = $data;
                return $ret;
            }

            
        } 
        catch (Exception $e) {
            if ($i == $retry){
                $ret['message'] = "S3: There was an error downloading the file '$file_remote' from bucket '$bucket'.\n".$e->getMessage();
                return $ret;
            }
            usleep(500000);
            continue;
        }
        break;
    }
}


function s3_set_env(){
    $version = 'latest';
    $region = 'ap-south-1';
    $retry = 5;
    $env_file = "/dhamma/htpasswd/s3-env.ini";
    $s3 = null;

    $s3_env = parse_ini_file($env_file);
    if (! $s3_env)
        return array('s3' => $s3, 'e' => "S3: Issue in parsing INI file '$env_file'.");

    foreach($s3_env as $key => $value){
            putenv("$key=$value");
    }

    if (! getenv('AWS_ACCESS_KEY_ID')){
        return array('s3' => $s3, 'e' => "S3: env 'AWS_ACCESS_KEY_ID' is not set.");
    }

    if (! getenv('AWS_SECRET_ACCESS_KEY')){
        return array('s3' => $s3, 'e' => "S3: env 'AWS_SECRET_ACCESS_KEY' is not set.");
    }

    if (! getenv('AWS_DEFAULT_REGION')){
        return array('s3' => $s3, 'e' => "S3: env 'AWS_DEFAULT_REGION' is not set.");
    }

    for ($i=1 ; $i <= $retry ; $i++){
        try{
            // Instantiate an Amazon S3 client.
            $s3 = new S3Client([
                'version' => $version,
                'region'  => $region,
            ]);
        }
        catch (Exception $e) {
            if ($i == $retry)
                return array('s3' => $s3, 'e' => $e->getMessage());
            usleep(500000);
            continue;
        }
        break;
    }
    return array('s3' => $s3, 'e' => null);
}

/* One S3 client for the head and copy helpers below: built once per process, then reused.
   Returns the client, or NULL when none can be built ($err then holds the reason). */
function s3_cached_client(&$err = NULL){
    static $client = NULL, $e = NULL, $done = FALSE;
    if (! $done){
        $r = s3_set_env();
        $client = $r['s3'];
        $e = $r['e'];
        $done = TRUE;
    }
    $err = $e;
    return $client;
}

/* TRUE when an S3 error means "that object does not exist" (HTTP 404, NotFound or NoSuchKey). */
function s3_error_is_not_found($e){
    if (method_exists($e, 'getStatusCode') && (int) $e->getStatusCode() == 404)
        return TRUE;
    if (method_exists($e, 'getAwsErrorCode') && in_array($e->getAwsErrorCode(), array('NotFound', 'NoSuchKey'), TRUE))
        return TRUE;
    return FALSE;
}

/* Size in bytes (int) of an object in the bucket. FALSE only when S3 says the object is not there.
   NULL for any other problem (no client, no permission, network); that is written to the S3Photo log.
   $client is for tests; leave it out to use the shared client. */
function s3_head_file($bucket, $key, $client = NULL){
    $err = NULL;
    if (! $client)
        $client = s3_cached_client($err);
    if (! $client){
        watchdog('S3Photo', 'S3: no client to check @key in bucket @bucket: @msg', array('@key' => $key, '@bucket' => $bucket, '@msg' => $err), WATCHDOG_ERROR);
        return NULL;
    }
    try {
        $r = $client->headObject(['Bucket' => $bucket, 'Key' => $key]);
        return (int) $r['ContentLength'];
    }
    catch (Exception $e) {
        if (s3_error_is_not_found($e))
            return FALSE;
        watchdog('S3Photo', 'S3: checking @key in bucket @bucket failed: @msg', array('@key' => $key, '@bucket' => $bucket, '@msg' => $e->getMessage()), WATCHDOG_ERROR);
        return NULL;
    }
}

/* Copy an object inside the bucket; the file never passes through this server.
   $client is for tests; leave it out to use the shared client. */
function s3_copy_file($bucket, $from, $to, $client = NULL){
    $ret = array('success' => false, 'message' => '');
    $err = NULL;
    if (! $client)
        $client = s3_cached_client($err);
    if (! $client){
        $ret['message'] = "S3: Not able to create s3 client for copying '$from'\n".$err;
        return $ret;
    }
    for ($i = 1; $i <= 5; $i++){
        try {
            $client->copyObject([
                'Bucket'     => $bucket,
                'Key'        => $to,
                'CopySource' => $bucket.'/'.str_replace('%2F', '/', rawurlencode($from)),
                'ACL'        => 'private',
            ]);
            $ret['success'] = true;
            return $ret;
        }
        catch (Exception $e) {
            if ($i == 5){
                $ret['message'] = "S3: copying '$from' to '$to' in bucket '$bucket' failed.\n".$e->getMessage();
                return $ret;
            }
            usleep(500000);
        }
    }
    return $ret;
}
