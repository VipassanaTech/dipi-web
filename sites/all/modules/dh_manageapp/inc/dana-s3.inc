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

